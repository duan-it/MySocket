/**
 * @file socket_sendrecv.c
 * @brief Socket 数据发送和接收实现
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 实现Socket的数据收发功能
 */

#include "socket_internal.h"

/**
 * 发送数据
 * @param sockfd Socket文件描述符
 * @param buf 发送数据缓冲区
 * @param len 数据长度
 * @param flags 发送标志
 * @return 发送的字节数，失败返回-1
 */
ssize_t mysocket_send(int sockfd, const void *buf, size_t len, int flags) {
    DEBUG_PRINT("发送数据: fd=%d, len=%zu", sockfd, len);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 参数验证 */
    if (!buf || len == 0) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查Socket状态 */
    if (sock->state != SS_CONNECTED) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查发送缓冲区空间 */
    size_t available = sock->send_buf_size - sock->send_buf_used;
    if (available == 0) {
        socket_set_error(MYSOCKET_EAGAIN);
        return -1;
    }
    
    /* 计算实际发送大小 */
    size_t send_len = (len > available) ? available : len;
    
    /* 写入发送缓冲区 */
    int written = socket_buffer_write(sock->send_buffer, &sock->send_buf_used,
                                     sock->send_buf_size, buf, send_len);
    if (written < 0) {
        socket_set_error(MYSOCKET_ERROR);
        return -1;
    }
    
    /* 尝试实际发送数据 */
    if (socket_flush_send_buffer(sock) < 0) {
        socket_set_error(MYSOCKET_ERROR);
        return -1;
    }
    
    DEBUG_PRINT("数据发送成功: fd=%d, sent=%d", sockfd, written);
    
    return written;
}

/**
 * 接收数据
 * @param sockfd Socket文件描述符
 * @param buf 接收数据缓冲区
 * @param len 缓冲区大小
 * @param flags 接收标志
 * @return 接收的字节数，失败返回-1
 */
ssize_t mysocket_recv(int sockfd, void *buf, size_t len, int flags) {
    DEBUG_PRINT("接收数据: fd=%d, len=%zu", sockfd, len);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 参数验证 */
    if (!buf || len == 0) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 检查Socket状态 */
    if (sock->state != SS_CONNECTED) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 尝试从网络接收数据到缓冲区 */
    socket_fill_recv_buffer(sock);
    
    /* 从接收缓冲区读取数据 */
    int read_len = socket_buffer_read(sock->recv_buffer, &sock->recv_buf_used,
                                     buf, len);
    if (read_len < 0) {
        socket_set_error(MYSOCKET_ERROR);
        return -1;
    }
    
    if (read_len == 0) {
        socket_set_error(MYSOCKET_EAGAIN);
        return -1;
    }
    
    DEBUG_PRINT("数据接收成功: fd=%d, recv=%d", sockfd, read_len);
    
    return read_len;
}

/**
 * 发送数据到指定地址（UDP）
 * @param sockfd Socket文件描述符
 * @param buf 发送数据缓冲区
 * @param len 数据长度
 * @param flags 发送标志
 * @param dest_addr 目标地址
 * @param addrlen 地址结构长度
 * @return 发送的字节数，失败返回-1
 */
ssize_t mysocket_sendto(int sockfd, const void *buf, size_t len, int flags,
                       const struct mysocket_addr *dest_addr, socklen_t addrlen) {
    DEBUG_PRINT("发送数据到指定地址: fd=%d, len=%zu", sockfd, len);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 参数验证 */
    if (!buf || len == 0 || !dest_addr || addrlen < sizeof(struct mysocket_addr_in)) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* UDP Socket才支持sendto */
    if (sock->type != SOCK_DGRAM) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 临时保存原对端地址 */
    struct mysocket_addr_in original_peer = sock->peer_addr;
    
    /* 设置目标地址 */
    if (socket_addr_copy(&sock->peer_addr, dest_addr, addrlen) < 0) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 发送数据 */
    ssize_t result = socket_send_udp_packet(sock, buf, len);
    
    /* 恢复原对端地址 */
    sock->peer_addr = original_peer;
    
    if (result < 0) {
        socket_set_error(MYSOCKET_ERROR);
        return -1;
    }
    
    DEBUG_PRINT("UDP数据发送成功: fd=%d, sent=%zd", sockfd, result);
    
    return result;
}

/**
 * 从指定地址接收数据（UDP）
 * @param sockfd Socket文件描述符
 * @param buf 接收数据缓冲区
 * @param len 缓冲区大小
 * @param flags 接收标志
 * @param src_addr 返回源地址
 * @param addrlen 地址结构长度
 * @return 接收的字节数，失败返回-1
 */
ssize_t mysocket_recvfrom(int sockfd, void *buf, size_t len, int flags,
                         struct mysocket_addr *src_addr, socklen_t *addrlen) {
    DEBUG_PRINT("从指定地址接收数据: fd=%d, len=%zu", sockfd, len);
    
    /* 查找Socket */
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 参数验证 */
    if (!buf || len == 0) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* UDP Socket才支持recvfrom */
    if (sock->type != SOCK_DGRAM) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 接收UDP数据包 */
    struct mysocket_addr_in peer_addr;
    ssize_t result = socket_recv_udp_packet(sock, buf, len, &peer_addr);
    
    if (result < 0) {
        socket_set_error(MYSOCKET_EAGAIN);
        return -1;
    }
    
    /* 返回源地址信息 */
    if (src_addr && addrlen && *addrlen >= sizeof(struct mysocket_addr_in)) {
        memcpy(src_addr, &peer_addr, sizeof(struct mysocket_addr_in));
        *addrlen = sizeof(struct mysocket_addr_in);
    }
    
    DEBUG_PRINT("UDP数据接收成功: fd=%d, recv=%zd", sockfd, result);
    
    return result;
}

/**
 * 刷新发送缓冲区（实际发送数据）
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int socket_flush_send_buffer(struct mysocket *sock) {
    if (!sock || sock->send_buf_used == 0) {
        return 0;
    }
    
    DEBUG_PRINT("刷新发送缓冲区: fd=%d, data=%zu", sock->fd, sock->send_buf_used);
    
    /* 根据协议类型处理 */
    if (sock->protocol == IPPROTO_TCP) {
        /* TCP数据发送 */
        if (tcp_send_data(sock, sock->send_buffer, sock->send_buf_used) < 0) {
            return -1;
        }
    } else if (sock->protocol == IPPROTO_UDP) {
        /* UDP数据发送 */
        if (socket_send_udp_packet(sock, sock->send_buffer, sock->send_buf_used) < 0) {
            return -1;
        }
    }
    
    /* 清空发送缓冲区 */
    sock->send_buf_used = 0;
    
    return 0;
}

/**
 * 填充接收缓冲区（从网络接收数据）
 * @param sock Socket指针
 * @return 接收的字节数，-1表示失败
 */
int socket_fill_recv_buffer(struct mysocket *sock) {
    if (!sock) return -1;
    
    /* 检查缓冲区空间 */
    size_t available = sock->recv_buf_size - sock->recv_buf_used;
    if (available == 0) {
        return 0;  /* 缓冲区已满 */
    }
    
    DEBUG_PRINT("填充接收缓冲区: fd=%d, space=%zu", sock->fd, available);
    
    /* 模拟从网络接收数据 */
    char temp_data[1024];
    size_t recv_len = 0;
    
    /* 根据协议类型接收数据 */
    if (sock->protocol == IPPROTO_TCP) {
        /* 模拟TCP数据接收 */
        recv_len = socket_simulate_tcp_receive(sock, temp_data, sizeof(temp_data));
    } else if (sock->protocol == IPPROTO_UDP) {
        /* 模拟UDP数据接收 */
        struct mysocket_addr_in peer_addr;
        recv_len = socket_recv_udp_packet(sock, temp_data, sizeof(temp_data), &peer_addr);
    }
    
    if (recv_len > 0) {
        /* 写入接收缓冲区 */
        size_t copy_len = (recv_len > available) ? available : recv_len;
        memcpy(sock->recv_buffer + sock->recv_buf_used, temp_data, copy_len);
        sock->recv_buf_used += copy_len;
        
        DEBUG_PRINT("接收数据写入缓冲区: fd=%d, len=%zu", sock->fd, copy_len);
        return copy_len;
    }
    
    return 0;
}

/**
 * 发送UDP数据包
 * @param sock Socket指针
 * @param data 数据
 * @param len 数据长度
 * @return 发送的字节数，-1表示失败
 */
ssize_t socket_send_udp_packet(struct mysocket *sock, const void *data, size_t len) {
    if (!sock || !data || len == 0) return -1;
    
    DEBUG_PRINT("发送UDP包: fd=%d, len=%zu, to=%08x:%d", 
                sock->fd, len, sock->peer_addr.sin_addr,
                mysocket_ntohs(sock->peer_addr.sin_port));
    
    /* 模拟UDP数据发送 */
    /* 在实际实现中，这里会构造UDP包并通过网络发送 */
    
    /* 简单模拟：如果目标地址有对应的接收Socket，将数据放入其接收缓冲区 */
    struct mysocket *target = socket_find_udp_receiver(&sock->peer_addr);
    if (target && target != sock) {
        size_t available = target->recv_buf_size - target->recv_buf_used;
        if (available > 0) {
            size_t copy_len = (len > available) ? available : len;
            memcpy(target->recv_buffer + target->recv_buf_used, data, copy_len);
            target->recv_buf_used += copy_len;
            
            DEBUG_PRINT("UDP数据传递到目标: target_fd=%d, len=%zu", target->fd, copy_len);
            return len;  /* 返回原始发送长度 */
        }
    }
    
    /* 如果没有找到目标或目标缓冲区满，仍然返回成功（UDP特性） */
    return len;
}

/**
 * 接收UDP数据包
 * @param sock Socket指针
 * @param buf 接收缓冲区
 * @param len 缓冲区大小
 * @param src_addr 返回源地址
 * @return 接收的字节数，-1表示失败
 */
ssize_t socket_recv_udp_packet(struct mysocket *sock, void *buf, size_t len,
                              struct mysocket_addr_in *src_addr) {
    if (!sock || !buf || len == 0) return -1;
    
    /* 模拟UDP数据接收 */
    /* 简单实现：检查接收缓冲区是否有数据 */
    if (sock->recv_buf_used == 0) {
        return 0;  /* 没有数据 */
    }
    
    /* 从缓冲区读取数据 */
    size_t copy_len = (len > sock->recv_buf_used) ? sock->recv_buf_used : len;
    memcpy(buf, sock->recv_buffer, copy_len);
    
    /* 移动剩余数据 */
    if (copy_len < sock->recv_buf_used) {
        memmove(sock->recv_buffer, sock->recv_buffer + copy_len, 
                sock->recv_buf_used - copy_len);
    }
    sock->recv_buf_used -= copy_len;
    
    /* 返回模拟的源地址 */
    if (src_addr) {
        src_addr->sin_family = AF_INET;
        src_addr->sin_addr = mysocket_htonl(0x7F000001);  /* 127.0.0.1 */
        src_addr->sin_port = mysocket_htons(rand() % 30000 + 32768);
    }
    
    DEBUG_PRINT("UDP数据接收: fd=%d, len=%zu", sock->fd, copy_len);
    
    return copy_len;
}

/**
 * 查找UDP接收Socket
 * @param addr 目标地址
 * @return Socket指针，未找到返回NULL
 */
struct mysocket* socket_find_udp_receiver(const struct mysocket_addr_in *addr) {
    if (!addr) return NULL;
    
    struct mysocket *current = g_socket_manager.socket_list;
    
    while (current != NULL) {
        /* 检查是否为UDP Socket */
        if (current->type == SOCK_DGRAM) {
            /* 检查地址匹配 */
            if (current->local_addr.sin_port == addr->sin_port) {
                /* 检查IP匹配（通配或精确） */
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
 * 模拟TCP数据接收
 * @param sock Socket指针
 * @param buf 接收缓冲区
 * @param len 缓冲区大小
 * @return 接收的字节数
 */
size_t socket_simulate_tcp_receive(struct mysocket *sock, void *buf, size_t len) {
    if (!sock || !buf) return 0;
    
    /* 简单模拟：生成一些测试数据 */
    static int counter = 0;
    counter++;
    
    if (counter % 10 == 0) {  /* 每10次调用生成一次数据 */
        char test_data[] = "Hello from TCP simulation!";
        size_t test_len = strlen(test_data);
        size_t copy_len = (len > test_len) ? test_len : len;
        
        memcpy(buf, test_data, copy_len);
        return copy_len;
    }
    
    return 0;  /* 没有数据 */
}