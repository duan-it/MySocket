/**
 * @file socket_accept_connect.c
 * @brief Socket accept和connect操作实现
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 实现Socket的连接接受和主动连接功能
 */

#include "socket_internal.h"
#include <time.h>

/**
 * 接受一个传入的连接
 * @param sockfd 监听Socket文件描述符
 * @param addr 返回客户端地址信息
 * @param addrlen 地址结构长度
 * @return 新连接的文件描述符，失败返回-1
 */
int mysocket_accept(int sockfd, struct mysocket_addr *addr, socklen_t *addrlen) {
    DEBUG_PRINT("接受连接: listen_fd=%d", sockfd);
    
    /* 查找监听Socket */
    struct mysocket *listen_sock = socket_find_by_fd(sockfd);
    if (!listen_sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查是否为监听Socket */
    if (listen_sock->state != SS_LISTENING) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 从监听队列中获取连接 */
    struct mysocket *new_sock = socket_listen_queue_remove(listen_sock);
    if (!new_sock) {
        /* 队列为空，尝试模拟接收新连接 */
        new_sock = socket_simulate_incoming_connection(listen_sock);
        if (!new_sock) {
            socket_set_error(MYSOCKET_EAGAIN);
            return -1;
        }
    }
    
    /* 设置新连接状态 */
    new_sock->state = SS_CONNECTED;
    if (new_sock->protocol == IPPROTO_TCP) {
        new_sock->tcp_state = TCP_ESTABLISHED;
    }
    
    /* 返回客户端地址信息 */
    if (addr && addrlen && *addrlen >= sizeof(struct mysocket_addr_in)) {
        memcpy(addr, &new_sock->peer_addr, sizeof(struct mysocket_addr_in));
        *addrlen = sizeof(struct mysocket_addr_in);
    }
    
    DEBUG_PRINT("连接接受成功: listen_fd=%d, new_fd=%d, peer=%08x:%d", 
                sockfd, new_sock->fd, new_sock->peer_addr.sin_addr,
                mysocket_ntohs(new_sock->peer_addr.sin_port));
    
    return new_sock->fd;
}

/**
 * 主动连接到指定地址
 * @param sockfd Socket文件描述符
 * @param addr 目标地址
 * @param addrlen 地址结构长度
 * @return 0成功，-1失败
 */
int mysocket_connect(int sockfd, const struct mysocket_addr *addr, socklen_t addrlen) {
    DEBUG_PRINT("主动连接: fd=%d", sockfd);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 参数验证 */
    if (!addr || addrlen < sizeof(struct mysocket_addr_in)) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查Socket状态 */
    if (sock->state != SS_UNCONNECTED) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 复制目标地址 */
    if (socket_addr_copy(&sock->peer_addr, addr, addrlen) < 0) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 如果本地地址未绑定，自动分配 */
    if (sock->local_addr.sin_port == 0) {
        if (socket_auto_bind(sock) < 0) {
            socket_set_error(MYSOCKET_ERROR);
            return -1;
        }
    }
    
    /* 更新Socket状态 */
    sock->state = SS_CONNECTING;
    
    /* TCP连接处理 */
    if (sock->protocol == IPPROTO_TCP) {
        /* 发送SYN包 */
        if (tcp_send_syn(sock) < 0) {
            sock->state = SS_UNCONNECTED;
            socket_set_error(MYSOCKET_ECONNREFUSED);
            return -1;
        }
        
        sock->tcp_state = TCP_SYN_SENT;
        
        /* 模拟连接建立过程 */
        if (socket_simulate_tcp_handshake(sock) < 0) {
            sock->state = SS_UNCONNECTED;
            sock->tcp_state = TCP_CLOSED;
            socket_set_error(MYSOCKET_ECONNREFUSED);
            return -1;
        }
        
        sock->state = SS_CONNECTED;
        sock->tcp_state = TCP_ESTABLISHED;
    } else {
        /* UDP连接（实际上只是记录对端地址） */
        sock->state = SS_CONNECTED;
    }
    
    DEBUG_PRINT("连接建立成功: fd=%d, peer=%08x:%d", 
                sockfd, sock->peer_addr.sin_addr,
                mysocket_ntohs(sock->peer_addr.sin_port));
    
    return MYSOCKET_OK;
}

/**
 * 自动绑定本地地址
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int socket_auto_bind(struct mysocket *sock) {
    if (!sock) return -1;
    
    /* 设置本地地址 */
    sock->local_addr.sin_family = AF_INET;
    sock->local_addr.sin_addr = 0;  /* INADDR_ANY */
    
    /* 分配临时端口（简单实现） */
    static uint16_t next_port = 32768;  /* 临时端口起始 */
    
    for (int i = 0; i < 1000; i++) {  /* 最多尝试1000次 */
        sock->local_addr.sin_port = mysocket_htons(next_port);
        
        /* 检查端口是否可用 */
        if (!socket_check_addr_in_use(&sock->local_addr, sock->fd)) {
            DEBUG_PRINT("自动绑定成功: fd=%d, port=%d", sock->fd, next_port);
            next_port++;
            if (next_port > 65535) next_port = 32768;
            return 0;
        }
        
        next_port++;
        if (next_port > 65535) next_port = 32768;
    }
    
    DEBUG_PRINT("自动绑定失败: fd=%d, 无可用端口", sock->fd);
    return -1;
}

/**
 * 模拟TCP三次握手过程
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int socket_simulate_tcp_handshake(struct mysocket *sock) {
    if (!sock) return -1;
    
    DEBUG_PRINT("模拟TCP握手: fd=%d", sock->fd);
    
    /* 检查目标地址是否可达（简单检查） */
    if (sock->peer_addr.sin_addr == 0 || sock->peer_addr.sin_port == 0) {
        return -1;
    }
    
    /* 模拟网络延迟 */
    struct timespec delay = {0, 1000000};  /* 1ms */
    nanosleep(&delay, NULL);
    
    /* 查找目标监听Socket */
    struct mysocket *target = socket_find_listening_socket(&sock->peer_addr);
    if (!target) {
        DEBUG_PRINT("目标地址无监听Socket: %08x:%d", 
                    sock->peer_addr.sin_addr,
                    mysocket_ntohs(sock->peer_addr.sin_port));
        return -1;
    }
    
    /* 模拟SYN-ACK响应 */
    DEBUG_PRINT("TCP握手成功: fd=%d", sock->fd);
    return 0;
}

/**
 * 模拟接收传入连接
 * @param listen_sock 监听Socket
 * @return 新连接Socket，失败返回NULL
 */
struct mysocket* socket_simulate_incoming_connection(struct mysocket *listen_sock) {
    if (!listen_sock) return NULL;
    
    DEBUG_PRINT("模拟传入连接: listen_fd=%d", listen_sock->fd);
    
    /* 创建新连接Socket */
    struct mysocket *new_sock = socket_create(listen_sock->family, 
                                             listen_sock->type, 
                                             listen_sock->protocol);
    if (!new_sock) {
        return NULL;
    }
    
    /* 添加到管理器 */
    if (socket_add_to_manager(new_sock) < 0) {
        socket_destroy(new_sock);
        return NULL;
    }
    
    /* 设置地址信息 */
    new_sock->local_addr = listen_sock->local_addr;
    
    /* 模拟客户端地址 */
    new_sock->peer_addr.sin_family = AF_INET;
    new_sock->peer_addr.sin_addr = mysocket_htonl(0x7F000001);  /* 127.0.0.1 */
    new_sock->peer_addr.sin_port = mysocket_htons(rand() % 30000 + 32768);
    
    /* 设置连接状态 */
    new_sock->state = SS_CONNECTED;
    if (new_sock->protocol == IPPROTO_TCP) {
        new_sock->tcp_state = TCP_ESTABLISHED;
    }
    
    DEBUG_PRINT("模拟连接创建成功: listen_fd=%d, new_fd=%d, peer=%08x:%d", 
                listen_sock->fd, new_sock->fd, 
                new_sock->peer_addr.sin_addr,
                mysocket_ntohs(new_sock->peer_addr.sin_port));
    
    return new_sock;
}

/**
 * 查找监听指定地址的Socket
 * @param addr 地址
 * @return 监听Socket指针，未找到返回NULL
 */
struct mysocket* socket_find_listening_socket(const struct mysocket_addr_in *addr) {
    if (!addr) return NULL;
    
    struct mysocket *current = g_socket_manager.socket_list;
    
    while (current != NULL) {
        /* 检查是否为监听状态 */
        if (current->state == SS_LISTENING) {
            /* 检查端口匹配 */
            if (current->local_addr.sin_port == addr->sin_port) {
                /* 检查地址匹配（通配地址或精确匹配） */
                if (current->local_addr.sin_addr == 0 || 
                    current->local_addr.sin_addr == addr->sin_addr) {
                    return current;
                }
            }
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * 检查连接是否可以接受
 * @param listen_sock 监听Socket
 * @param peer_addr 客户端地址
 * @return 1可接受，0不可接受
 */
int socket_can_accept_connection(struct mysocket *listen_sock, 
                               const struct mysocket_addr_in *peer_addr) {
    if (!listen_sock || !peer_addr) return 0;
    
    /* 检查监听状态 */
    if (listen_sock->state != SS_LISTENING) {
        return 0;
    }
    
    /* 检查队列是否已满 */
    if (listen_sock->listen_count >= listen_sock->listen_backlog) {
        return 0;
    }
    
    /* 可以添加其他检查逻辑，如访问控制等 */
    
    return 1;
}

/**
 * 获取连接状态信息
 * @param sockfd Socket文件描述符
 * @return Socket状态，-1表示无效
 */
int mysocket_get_socket_state(int sockfd) {
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        return -1;
    }
    
    return sock->state;
}