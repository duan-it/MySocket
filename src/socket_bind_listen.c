/**
 * @file socket_bind_listen.c
 * @brief Socket bind和listen操作实现
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 实现Socket的地址绑定和监听功能
 */

#include "socket_internal.h"
#include <arpa/inet.h>

/**
 * 绑定Socket到指定地址
 * @param sockfd Socket文件描述符
 * @param addr 要绑定的地址
 * @param addrlen 地址结构长度
 * @return 0成功，-1失败
 */
int mysocket_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    DEBUG_PRINT("绑定Socket: fd=%d", sockfd);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 参数验证 */
    if (!addr || addrlen < sizeof(struct sockaddr)) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查Socket状态 */
    if (sock->state != SS_UNCONNECTED) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 复制地址信息 */
    if (socket_addr_copy(&sock->local_addr, addr, addrlen) < 0) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查地址是否已被使用（简单检查） */
    if (!socket_addr_is_any(&sock->local_addr)) {
        if (socket_check_addr_in_use(&sock->local_addr)) {
            socket_set_error(MYSOCKET_EADDRINUSE);
            return -1;
        }
    }
    
    /* 更新Socket状态 */
    sock->state = SS_UNCONNECTED;  /* 绑定后仍然是未连接状态 */
    
    DEBUG_PRINT("Socket绑定成功: fd=%d, addr=%08x:%d", 
                sockfd, sock->local_addr.sin_addr, 
                mysocket_ntohs(sock->local_addr.sin_port));
    
    return MYSOCKET_OK;
}

/**
 * 使Socket进入监听状态
 * @param sockfd Socket文件描述符
 * @param backlog 最大挂起连接数
 * @return 0成功，-1失败
 */
int mysocket_listen(int sockfd, int backlog) {
    DEBUG_PRINT("Socket进入监听: fd=%d, backlog=%d", sockfd, backlog);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 只有流式Socket可以监听 */
    if (sock->type != SOCK_STREAM) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 必须先绑定地址 */
    if (sock->local_addr.sin_port == 0) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查Socket状态 */
    if (sock->state != SS_UNCONNECTED) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 设置backlog */
    if (backlog <= 0) {
        backlog = DEFAULT_LISTEN_BACKLOG;
    } else if (backlog > DEFAULT_LISTEN_BACKLOG) {
        backlog = DEFAULT_LISTEN_BACKLOG;
    }
    
    /* 分配监听队列 */
    if (sock->listen_queue) {
        free(sock->listen_queue);
    }
    
    sock->listen_queue = calloc(backlog, sizeof(struct mysocket*));
    if (!sock->listen_queue) {
        socket_set_error(MYSOCKET_ERROR);
        return -1;
    }
    
    sock->listen_backlog = backlog;
    sock->listen_count = 0;
    
    /* 更新Socket状态 */
    sock->state = SS_LISTENING;
    if (sock->protocol == IPPROTO_TCP) {
        sock->tcp_state = TCP_LISTEN;
    }
    
    DEBUG_PRINT("Socket监听成功: fd=%d, backlog=%d, addr=%08x:%d", 
                sockfd, backlog, sock->local_addr.sin_addr,
                mysocket_ntohs(sock->local_addr.sin_port));
    
    return MYSOCKET_OK;
}

/**
 * 复制地址结构
 * @param dst 目标地址
 * @param src 源地址
 * @param addrlen 地址长度
 * @return 0成功，-1失败
 */
int socket_addr_copy(struct sockaddr_in *dst, const struct sockaddr *src, socklen_t addrlen) {
    if (!dst || !src) return -1;
    
    /* 检查地址族 */
    if (src->sa_family != AF_INET) {
        return -1;
    }
    
    /* 检查长度 */
    if (addrlen < sizeof(struct sockaddr_in)) {
        return -1;
    }
    
    /* 复制地址 */
    memcpy(dst, src, sizeof(struct sockaddr_in));
    
    return 0;
}

/**
 * 比较两个地址是否相同
 * @param addr1 地址1
 * @param addr2 地址2
 * @return 1相同，0不同
 */
int socket_addr_compare(const struct sockaddr_in *addr1, const struct sockaddr_in *addr2) {
    if (!addr1 || !addr2) return 0;
    
    return (addr1->sin_family == addr2->sin_family) &&
           (addr1->sin_port == addr2->sin_port) &&
           (addr1->sin_addr == addr2->sin_addr);
}

/**
 * 检查地址是否为通配地址
 * @param addr 地址结构
 * @return 1是通配地址，0不是
 */
int socket_addr_is_any(const struct sockaddr_in *addr) {
    if (!addr) return 0;
    
    return (addr->sin_addr == 0);  /* INADDR_ANY = 0 */
}

/**
 * 检查地址是否已被使用
 * @param addr 要检查的地址
 * @return 1已使用，0未使用
 */
int socket_check_addr_in_use(const struct sockaddr_in *addr) {
    if (!addr) return 0;
    
    /* 遍历所有Socket，检查是否有相同的绑定地址 */
    struct mysocket *current = g_socket_manager.socket_list;
    
    while (current != NULL) {
        /* 跳过未绑定的Socket */
        if (current->local_addr.sin_port == 0) {
            current = current->next;
            continue;
        }
        
        /* 检查端口冲突 */
        if (current->local_addr.sin_port == addr->sin_port) {
            /* 如果其中一个是通配地址，则冲突 */
            if (current->local_addr.sin_addr == 0 || addr->sin_addr == 0) {
                return 1;
            }
            
            /* 检查IP地址冲突 */
            if (current->local_addr.sin_addr == addr->sin_addr) {
                return 1;
            }
        }
        
        current = current->next;
    }
    
    return 0;
}

/**
 * 在监听队列中添加连接
 * @param listen_sock 监听Socket
 * @param new_sock 新连接Socket
 * @return 0成功，-1失败
 */
int socket_listen_queue_add(struct mysocket *listen_sock, struct mysocket *new_sock) {
    if (!listen_sock || !new_sock) return -1;
    
    /* 检查队列是否已满 */
    if (listen_sock->listen_count >= listen_sock->listen_backlog) {
        DEBUG_PRINT("监听队列已满: fd=%d, count=%d, backlog=%d",
                    listen_sock->fd, listen_sock->listen_count, 
                    listen_sock->listen_backlog);
        return -1;
    }
    
    /* 添加到队列 */
    listen_sock->listen_queue[listen_sock->listen_count] = new_sock;
    listen_sock->listen_count++;
    
    DEBUG_PRINT("连接添加到监听队列: listen_fd=%d, new_fd=%d, count=%d",
                listen_sock->fd, new_sock->fd, listen_sock->listen_count);
    
    return 0;
}

/**
 * 从监听队列中移除连接
 * @param listen_sock 监听Socket
 * @return 移除的Socket指针，NULL表示队列为空
 */
struct mysocket* socket_listen_queue_remove(struct mysocket *listen_sock) {
    if (!listen_sock || listen_sock->listen_count == 0) {
        return NULL;
    }
    
    /* 取出第一个连接（FIFO） */
    struct mysocket *sock = listen_sock->listen_queue[0];
    
    /* 移动其他连接 */
    for (int i = 1; i < listen_sock->listen_count; i++) {
        listen_sock->listen_queue[i-1] = listen_sock->listen_queue[i];
    }
    
    listen_sock->listen_count--;
    listen_sock->listen_queue[listen_sock->listen_count] = NULL;
    
    DEBUG_PRINT("连接从监听队列移除: listen_fd=%d, removed_fd=%d, remaining=%d",
                listen_sock->fd, sock->fd, listen_sock->listen_count);
    
    return sock;
}

/**
 * 获取监听队列状态
 * @param listen_sock 监听Socket
 * @param count 返回当前连接数
 * @param backlog 返回最大连接数
 * @return 0成功，-1失败
 */
int socket_listen_queue_status(struct mysocket *listen_sock, int *count, int *backlog) {
    if (!listen_sock) return -1;
    
    if (count) *count = listen_sock->listen_count;
    if (backlog) *backlog = listen_sock->listen_backlog;
    
    return 0;
}