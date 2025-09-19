/**
 * @file socket_core.c
 * @brief Socket 核心实现
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 包含Socket的基本操作实现，模仿Linux内核实现
 */

#include "socket_internal.h"
#include <pthread.h>

/* 全局Socket管理器 */
struct socket_manager g_socket_manager = {0};

/* 线程锁，保护全局数据结构 */
static pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 错误处理 */
static __thread int last_error = 0;

/**
 * 初始化Socket系统
 * @return 0成功，-1失败
 */
int mysocket_init(void) {
    DEBUG_PRINT("初始化Socket系统");
    
    pthread_mutex_lock(&socket_mutex);
    
    /* 初始化管理器 */
    g_socket_manager.socket_list = NULL;
    g_socket_manager.next_fd = 3;  /* 从3开始，0,1,2被标准流占用 */
    g_socket_manager.total_sockets = 0;
    
    pthread_mutex_unlock(&socket_mutex);
    
    DEBUG_PRINT("Socket系统初始化完成");
    return MYSOCKET_OK;
}

/**
 * 清理Socket系统
 */
void mysocket_cleanup(void) {
    DEBUG_PRINT("清理Socket系统");
    
    pthread_mutex_lock(&socket_mutex);
    
    struct mysocket *current = g_socket_manager.socket_list;
    while (current != NULL) {
        struct mysocket *next = current->next;
        socket_destroy(current);
        current = next;
    }
    
    g_socket_manager.socket_list = NULL;
    g_socket_manager.total_sockets = 0;
    
    pthread_mutex_unlock(&socket_mutex);
    
    DEBUG_PRINT("Socket系统清理完成");
}

/**
 * 创建Socket
 * @param domain 协议族 (AF_INET, AF_UNIX等)
 * @param type Socket类型 (SOCK_STREAM, SOCK_DGRAM等)
 * @param protocol 协议 (IPPROTO_TCP, IPPROTO_UDP等)
 * @return Socket文件描述符，失败返回-1
 */
int mysocket_socket(int domain, int type, int protocol) {
    DEBUG_PRINT("创建Socket: domain=%d, type=%d, protocol=%d", domain, type, protocol);
    
    /* 参数验证 */
    if (domain != AF_INET && domain != AF_UNIX) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    if (type != SOCK_STREAM && type != SOCK_DGRAM && type != SOCK_RAW) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 协议自动推导 */
    if (protocol == 0) {
        if (type == SOCK_STREAM) {
            protocol = IPPROTO_TCP;
        } else if (type == SOCK_DGRAM) {
            protocol = IPPROTO_UDP;
        }
    }
    
    /* 创建Socket结构 */
    struct mysocket *sock = socket_create(domain, type, protocol);
    if (!sock) {
        socket_set_error(MYSOCKET_ERROR);
        return -1;
    }
    
    /* 添加到管理器 */
    if (socket_add_to_manager(sock) < 0) {
        socket_destroy(sock);
        socket_set_error(MYSOCKET_ERROR);
        return -1;
    }
    
    DEBUG_PRINT("Socket创建成功，fd=%d", sock->fd);
    return sock->fd;
}

/**
 * 关闭Socket
 * @param sockfd Socket文件描述符
 * @return 0成功，-1失败
 */
int mysocket_close(int sockfd) {
    DEBUG_PRINT("关闭Socket: fd=%d", sockfd);
    
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 如果是TCP连接，需要优雅关闭 */
    if (sock->type == SOCK_STREAM && sock->state == SS_CONNECTED) {
        tcp_send_fin(sock);
        sock->tcp_state = TCP_FIN_WAIT1;
    }
    
    /* 从管理器中移除 */
    socket_remove_from_manager(sock);
    
    /* 销毁Socket */
    socket_destroy(sock);
    
    DEBUG_PRINT("Socket关闭完成: fd=%d", sockfd);
    return MYSOCKET_OK;
}

/**
 * 根据文件描述符查找Socket
 * @param fd 文件描述符
 * @return Socket指针，未找到返回NULL
 */
struct mysocket* socket_find_by_fd(int fd) {
    pthread_mutex_lock(&socket_mutex);
    
    struct mysocket *current = g_socket_manager.socket_list;
    while (current != NULL) {
        if (current->fd == fd) {
            pthread_mutex_unlock(&socket_mutex);
            return current;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&socket_mutex);
    return NULL;
}

/**
 * 创建Socket结构体
 * @param domain 协议族
 * @param type Socket类型
 * @param protocol 协议
 * @return Socket指针，失败返回NULL
 */
struct mysocket* socket_create(int domain, int type, int protocol) {
    struct mysocket *sock = calloc(1, sizeof(struct mysocket));
    if (!sock) {
        return NULL;
    }
    
    /* 分配文件描述符 */
    pthread_mutex_lock(&socket_mutex);
    sock->fd = g_socket_manager.next_fd++;
    pthread_mutex_unlock(&socket_mutex);
    
    /* 初始化基本属性 */
    sock->family = domain;
    sock->type = type;
    sock->protocol = protocol;
    sock->state = SS_UNCONNECTED;
    sock->tcp_state = TCP_CLOSED;
    
    /* 初始化地址 */
    memset(&sock->local_addr, 0, sizeof(sock->local_addr));
    memset(&sock->peer_addr, 0, sizeof(sock->peer_addr));
    sock->local_addr.sin_family = domain;
    sock->peer_addr.sin_family = domain;
    
    /* 初始化缓冲区 */
    if (socket_buffer_init(sock) < 0) {
        free(sock);
        return NULL;
    }
    
    /* 初始化监听队列 */
    sock->listen_queue = NULL;
    sock->listen_backlog = 0;
    sock->listen_count = 0;
    
    sock->next = NULL;
    
    DEBUG_PRINT("Socket结构创建成功: fd=%d, family=%d, type=%d, protocol=%d",
                sock->fd, domain, type, protocol);
    
    return sock;
}

/**
 * 销毁Socket结构体
 * @param sock Socket指针
 */
void socket_destroy(struct mysocket *sock) {
    if (!sock) return;
    
    DEBUG_PRINT("销毁Socket结构: fd=%d", sock->fd);
    
    /* 清理缓冲区 */
    socket_buffer_cleanup(sock);
    
    /* 清理监听队列 */
    if (sock->listen_queue) {
        free(sock->listen_queue);
    }
    
    /* 释放结构体 */
    free(sock);
}

/**
 * 将Socket添加到管理器
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int socket_add_to_manager(struct mysocket *sock) {
    if (!sock) return -1;
    
    pthread_mutex_lock(&socket_mutex);
    
    /* 添加到链表头部 */
    sock->next = g_socket_manager.socket_list;
    g_socket_manager.socket_list = sock;
    g_socket_manager.total_sockets++;
    
    pthread_mutex_unlock(&socket_mutex);
    
    DEBUG_PRINT("Socket添加到管理器: fd=%d, 总数=%d", 
                sock->fd, g_socket_manager.total_sockets);
    return 0;
}

/**
 * 从管理器中移除Socket
 * @param sock Socket指针
 */
void socket_remove_from_manager(struct mysocket *sock) {
    if (!sock) return;
    
    pthread_mutex_lock(&socket_mutex);
    
    struct mysocket *current = g_socket_manager.socket_list;
    struct mysocket *prev = NULL;
    
    while (current != NULL) {
        if (current == sock) {
            if (prev) {
                prev->next = current->next;
            } else {
                g_socket_manager.socket_list = current->next;
            }
            g_socket_manager.total_sockets--;
            break;
        }
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(&socket_mutex);
    
    DEBUG_PRINT("Socket从管理器移除: fd=%d, 剩余=%d", 
                sock->fd, g_socket_manager.total_sockets);
}

/**
 * 设置错误码
 * @param error_code 错误码
 */
void socket_set_error(int error_code) {
    last_error = error_code;
}

/**
 * 获取错误码
 * @return 错误码
 */
int socket_get_error(void) {
    return last_error;
}

/**
 * 获取错误信息
 * @param error_code 错误码
 * @return 错误信息字符串
 */
const char* mysocket_strerror(int error_code) {
    switch (error_code) {
        case MYSOCKET_OK:
            return "成功";
        case MYSOCKET_ERROR:
            return "一般错误";
        case MYSOCKET_EAGAIN:
            return "资源暂时不可用";
        case MYSOCKET_EINVAL:
            return "无效参数";
        case MYSOCKET_EADDRINUSE:
            return "地址已被使用";
        case MYSOCKET_ECONNREFUSED:
            return "连接被拒绝";
        case MYSOCKET_ETIMEDOUT:
            return "连接超时";
        default:
            return "未知错误";
    }
}

/**
 * 打印Socket信息（调试用）
 * @param sockfd Socket文件描述符
 */
void mysocket_print_socket_info(int sockfd) {
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        printf("Socket fd=%d 不存在\n", sockfd);
        return;
    }
    
    printf("Socket信息 fd=%d:\n", sockfd);
    printf("  协议族: %d\n", sock->family);
    printf("  类型: %d\n", sock->type);
    printf("  协议: %d\n", sock->protocol);
    printf("  状态: %d\n", sock->state);
    printf("  TCP状态: %d\n", sock->tcp_state);
    printf("  本地地址: %08x:%d\n", sock->local_addr.sin_addr, sock->local_addr.sin_port);
    printf("  对端地址: %08x:%d\n", sock->peer_addr.sin_addr, sock->peer_addr.sin_port);
    printf("  发送缓冲区: %zu/%zu\n", sock->send_buf_used, sock->send_buf_size);
    printf("  接收缓冲区: %zu/%zu\n", sock->recv_buf_used, sock->recv_buf_size);
}