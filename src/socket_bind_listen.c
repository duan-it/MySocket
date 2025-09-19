/**
 * @file socket_bind_listen.c
 * @brief Socket bind和listen操作实现
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 实现Socket的地址绑定和监听功能
 */

#include "socket_internal.h"

/**
 * 绑定Socket到指定地址
 * @param sockfd Socket文件描述符
 * @param addr 要绑定的地址
 * @param addrlen 地址结构长度
 * @return 0成功，-1失败
 */
int mysocket_bind(int sockfd, const struct mysocket_addr *addr, socklen_t addrlen) {
    DEBUG_PRINT("绑定Socket: fd=%d", sockfd);
    DEBUG_PRINT("开始参数检查: addr=%p, addrlen=%u", addr, addrlen);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        DEBUG_PRINT("错误: Socket查找失败");
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    DEBUG_PRINT("Socket查找成功: fd=%d", sockfd);
    
    /* 参数验证 */
    if (!addr || addrlen < sizeof(struct mysocket_addr_in)) {
        DEBUG_PRINT("错误: 参数验证失败, addr=%p, addrlen=%u, 需要>=%zu", addr, addrlen, sizeof(struct mysocket_addr_in));
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    DEBUG_PRINT("参数验证通过");
    
    /* 检查Socket状态 */
    if (sock->state != SS_UNCONNECTED) {
        DEBUG_PRINT("错误: Socket状态不正确, 当前state=%d, 需要=%d", sock->state, SS_UNCONNECTED);
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    DEBUG_PRINT("Socket状态检查通过, state=%d", sock->state);
    
    /* 先检查地址是否已被使用（在复制之前检查） */
    DEBUG_PRINT("检查地址冲突");
    const struct mysocket_addr_in *addr_in = (const struct mysocket_addr_in *)addr;
    if (!socket_addr_is_any(addr_in)) {
        DEBUG_PRINT("非通配地址，检查是否已被使用");
        if (socket_check_addr_in_use(addr_in, sock->fd)) {
            DEBUG_PRINT("错误: 地址已被使用");
            socket_set_error(MYSOCKET_EADDRINUSE);
            return -1;
        }
        DEBUG_PRINT("地址可用");
    } else {
        DEBUG_PRINT("使用通配地址，跳过冲突检查");
    }
    
    /* 复制地址信息 */
    DEBUG_PRINT("开始复制地址信息");
    if (socket_addr_copy(&sock->local_addr, addr, addrlen) < 0) {
        DEBUG_PRINT("错误: 地址复制失败");
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    DEBUG_PRINT("地址复制成功");
    
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
        DEBUG_PRINT("错误: Socket查找失败");
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
int socket_addr_copy(struct mysocket_addr_in *dst, const struct mysocket_addr *src, socklen_t addrlen) {
    DEBUG_PRINT("socket_addr_copy: dst=%p, src=%p, addrlen=%u", dst, src, addrlen);
    
    if (!dst || !src) {
        DEBUG_PRINT("socket_addr_copy: 空指针错误");
        return -1;
    }
    
    /* 检查地址族 */
    DEBUG_PRINT("socket_addr_copy: 检查地址族, src->sa_family=%u, AF_INET=%u", src->sa_family, AF_INET);
    if (src->sa_family != AF_INET) {
        DEBUG_PRINT("socket_addr_copy: 地址族不匹配");
        return -1;
    }
    
    /* 检查长度 */
    DEBUG_PRINT("socket_addr_copy: 检查长度, addrlen=%u, 需要>=%zu", addrlen, sizeof(struct mysocket_addr_in));
    if (addrlen < sizeof(struct mysocket_addr_in)) {
        DEBUG_PRINT("socket_addr_copy: 长度不足");
        return -1;
    }
    
    /* 复制地址 */
    DEBUG_PRINT("socket_addr_copy: 开始复制地址");
    const struct mysocket_addr_in *src_in = (const struct mysocket_addr_in *)src;
    DEBUG_PRINT("socket_addr_copy: 源地址 family=%u, port=%u, addr=0x%x", 
                src_in->sin_family, src_in->sin_port, src_in->sin_addr);
    memcpy(dst, src, sizeof(struct mysocket_addr_in));
    DEBUG_PRINT("socket_addr_copy: 地址复制完成");
    
    return 0;
}

/**
 * 比较两个地址是否相同
 * @param addr1 地址1
 * @param addr2 地址2
 * @return 1相同，0不同
 */
int socket_addr_compare(const struct mysocket_addr_in *addr1, const struct mysocket_addr_in *addr2) {
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
int socket_addr_is_any(const struct mysocket_addr_in *addr) {
    if (!addr) return 0;
    
    return (addr->sin_addr == 0);  /* INADDR_ANY = 0 */
}

/**
 * 检查地址是否已被使用
 * @param addr 要检查的地址
 * @param exclude_fd 要排除检查的socket fd（-1表示不排除）
 * @return 1已使用，0未使用
 */
int socket_check_addr_in_use(const struct mysocket_addr_in *addr, int exclude_fd) {
    DEBUG_PRINT("socket_check_addr_in_use: 检查地址 port=%u, addr=0x%x, 排除fd=%d", addr->sin_port, addr->sin_addr, exclude_fd);
    if (!addr) return 0;
    
    /* 遍历所有Socket，检查是否有相同的绑定地址 */
    struct mysocket *current = g_socket_manager.socket_list;
    DEBUG_PRINT("socket_check_addr_in_use: 开始遍历Socket列表");
    
    while (current != NULL) {
        DEBUG_PRINT("socket_check_addr_in_use: 检查Socket fd=%d, local_port=%u, local_addr=0x%x", 
                    current->fd, current->local_addr.sin_port, current->local_addr.sin_addr);
        
        /* 跳过要排除的Socket */
        if (exclude_fd >= 0 && current->fd == exclude_fd) {
            DEBUG_PRINT("socket_check_addr_in_use: 跳过排除的Socket fd=%d", current->fd);
            current = current->next;
            continue;
        }
        
        /* 跳过未绑定的Socket */
        if (current->local_addr.sin_port == 0) {
            DEBUG_PRINT("socket_check_addr_in_use: 跳过未绑定的Socket fd=%d", current->fd);
            current = current->next;
            continue;
        }
        
        /* 检查端口冲突 */
        if (current->local_addr.sin_port == addr->sin_port) {
            DEBUG_PRINT("socket_check_addr_in_use: 发现端口冲突! fd=%d使用相同端口%u", current->fd, addr->sin_port);
            /* 如果其中一个是通配地址，则冲突 */
            if (current->local_addr.sin_addr == 0 || addr->sin_addr == 0) {
                DEBUG_PRINT("socket_check_addr_in_use: 通配地址冲突");
                return 1;
            }
            
            /* 检查IP地址冲突 */
            if (current->local_addr.sin_addr == addr->sin_addr) {
                DEBUG_PRINT("socket_check_addr_in_use: IP地址冲突");
                return 1;
            }
            DEBUG_PRINT("socket_check_addr_in_use: 端口相同但IP不同，无冲突");
        }
        
        current = current->next;
    }
    
    DEBUG_PRINT("socket_check_addr_in_use: 未发现地址冲突");
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