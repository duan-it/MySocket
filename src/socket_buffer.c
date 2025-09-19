/**
 * @file socket_buffer.c
 * @brief Socket 缓冲区管理实现
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 实现Socket的发送和接收缓冲区管理
 */

#include "socket_internal.h"

/**
 * 初始化Socket缓冲区
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int socket_buffer_init(struct mysocket *sock) {
    if (!sock) return -1;
    
    /* 分配发送缓冲区 */
    sock->send_buffer = malloc(DEFAULT_SEND_BUFFER_SIZE);
    if (!sock->send_buffer) {
        return -1;
    }
    
    /* 分配接收缓冲区 */
    sock->recv_buffer = malloc(DEFAULT_RECV_BUFFER_SIZE);
    if (!sock->recv_buffer) {
        free(sock->send_buffer);
        sock->send_buffer = NULL;
        return -1;
    }
    
    /* 初始化缓冲区参数 */
    sock->send_buf_size = DEFAULT_SEND_BUFFER_SIZE;
    sock->recv_buf_size = DEFAULT_RECV_BUFFER_SIZE;
    sock->send_buf_used = 0;
    sock->recv_buf_used = 0;
    
    DEBUG_PRINT("Socket缓冲区初始化成功: fd=%d, send=%zu, recv=%zu", 
                sock->fd, sock->send_buf_size, sock->recv_buf_size);
    
    return 0;
}

/**
 * 清理Socket缓冲区
 * @param sock Socket指针
 */
void socket_buffer_cleanup(struct mysocket *sock) {
    if (!sock) return;
    
    if (sock->send_buffer) {
        free(sock->send_buffer);
        sock->send_buffer = NULL;
    }
    
    if (sock->recv_buffer) {
        free(sock->recv_buffer);
        sock->recv_buffer = NULL;
    }
    
    sock->send_buf_size = 0;
    sock->recv_buf_size = 0;
    sock->send_buf_used = 0;
    sock->recv_buf_used = 0;
    
    DEBUG_PRINT("Socket缓冲区清理完成: fd=%d", sock->fd);
}

/**
 * 向缓冲区写入数据
 * @param buffer 缓冲区指针
 * @param used 已使用大小指针
 * @param total 缓冲区总大小
 * @param data 要写入的数据
 * @param len 数据长度
 * @return 实际写入的字节数，-1表示失败
 */
int socket_buffer_write(char *buffer, size_t *used, size_t total, 
                       const void *data, size_t len) {
    if (!buffer || !used || !data) return -1;
    
    /* 检查缓冲区空间 */
    size_t available = total - *used;
    if (available == 0) {
        return 0;  /* 缓冲区已满 */
    }
    
    /* 计算实际写入大小 */
    size_t write_len = (len > available) ? available : len;
    
    /* 写入数据 */
    memcpy(buffer + *used, data, write_len);
    *used += write_len;
    
    DEBUG_PRINT("缓冲区写入: len=%zu, used=%zu/%zu", write_len, *used, total);
    
    return write_len;
}

/**
 * 从缓冲区读取数据
 * @param buffer 缓冲区指针
 * @param used 已使用大小指针
 * @param data 读取数据的目标缓冲区
 * @param len 要读取的数据长度
 * @return 实际读取的字节数，-1表示失败
 */
int socket_buffer_read(char *buffer, size_t *used, void *data, size_t len) {
    if (!buffer || !used || !data) return -1;
    
    /* 检查缓冲区数据 */
    if (*used == 0) {
        return 0;  /* 缓冲区为空 */
    }
    
    /* 计算实际读取大小 */
    size_t read_len = (len > *used) ? *used : len;
    
    /* 读取数据 */
    memcpy(data, buffer, read_len);
    
    /* 移动剩余数据到缓冲区开头 */
    if (read_len < *used) {
        memmove(buffer, buffer + read_len, *used - read_len);
    }
    
    *used -= read_len;
    
    DEBUG_PRINT("缓冲区读取: len=%zu, remaining=%zu", read_len, *used);
    
    return read_len;
}

/**
 * 扩展缓冲区大小
 * @param sock Socket指针
 * @param send_size 新的发送缓冲区大小，0表示不改变
 * @param recv_size 新的接收缓冲区大小，0表示不改变
 * @return 0成功，-1失败
 */
int socket_buffer_resize(struct mysocket *sock, size_t send_size, size_t recv_size) {
    if (!sock) return -1;
    
    /* 扩展发送缓冲区 */
    if (send_size > 0 && send_size != sock->send_buf_size) {
        char *new_send_buffer = realloc(sock->send_buffer, send_size);
        if (!new_send_buffer) {
            return -1;
        }
        
        sock->send_buffer = new_send_buffer;
        sock->send_buf_size = send_size;
        
        /* 如果新大小小于已使用大小，截断数据 */
        if (sock->send_buf_used > send_size) {
            sock->send_buf_used = send_size;
        }
        
        DEBUG_PRINT("发送缓冲区扩展: fd=%d, new_size=%zu", sock->fd, send_size);
    }
    
    /* 扩展接收缓冲区 */
    if (recv_size > 0 && recv_size != sock->recv_buf_size) {
        char *new_recv_buffer = realloc(sock->recv_buffer, recv_size);
        if (!new_recv_buffer) {
            return -1;
        }
        
        sock->recv_buffer = new_recv_buffer;
        sock->recv_buf_size = recv_size;
        
        /* 如果新大小小于已使用大小，截断数据 */
        if (sock->recv_buf_used > recv_size) {
            sock->recv_buf_used = recv_size;
        }
        
        DEBUG_PRINT("接收缓冲区扩展: fd=%d, new_size=%zu", sock->fd, recv_size);
    }
    
    return 0;
}

/**
 * 清空缓冲区内容
 * @param sock Socket指针
 * @param clear_send 是否清空发送缓冲区
 * @param clear_recv 是否清空接收缓冲区
 */
void socket_buffer_clear(struct mysocket *sock, int clear_send, int clear_recv) {
    if (!sock) return;
    
    if (clear_send && sock->send_buffer) {
        sock->send_buf_used = 0;
        DEBUG_PRINT("发送缓冲区已清空: fd=%d", sock->fd);
    }
    
    if (clear_recv && sock->recv_buffer) {
        sock->recv_buf_used = 0;
        DEBUG_PRINT("接收缓冲区已清空: fd=%d", sock->fd);
    }
}

/**
 * 获取缓冲区状态信息
 * @param sock Socket指针
 * @param send_used 返回发送缓冲区已使用大小
 * @param send_free 返回发送缓冲区剩余空间
 * @param recv_used 返回接收缓冲区已使用大小
 * @param recv_free 返回接收缓冲区剩余空间
 * @return 0成功，-1失败
 */
int socket_buffer_status(struct mysocket *sock, 
                        size_t *send_used, size_t *send_free,
                        size_t *recv_used, size_t *recv_free) {
    if (!sock) return -1;
    
    if (send_used) *send_used = sock->send_buf_used;
    if (send_free) *send_free = sock->send_buf_size - sock->send_buf_used;
    if (recv_used) *recv_used = sock->recv_buf_used;
    if (recv_free) *recv_free = sock->recv_buf_size - sock->recv_buf_used;
    
    return 0;
}

/**
 * 检查缓冲区是否有足够空间
 * @param sock Socket指针
 * @param send_need 发送需要的空间
 * @param recv_need 接收需要的空间
 * @return 1表示有足够空间，0表示空间不足
 */
int socket_buffer_has_space(struct mysocket *sock, size_t send_need, size_t recv_need) {
    if (!sock) return 0;
    
    int send_ok = (send_need == 0) || 
                  ((sock->send_buf_size - sock->send_buf_used) >= send_need);
    
    int recv_ok = (recv_need == 0) || 
                  ((sock->recv_buf_size - sock->recv_buf_used) >= recv_need);
    
    return send_ok && recv_ok;
}