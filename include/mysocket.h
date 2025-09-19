/**
 * @file mysocket.h
 * @brief Socket 学习项目主头文件
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 这个项目模仿Linux内核实现，用于学习Socket底层原理
 */

#ifndef MYSOCKET_H
#define MYSOCKET_H

#include <stdint.h>
#include <sys/types.h>

/* 协议族定义 - 模仿Linux内核 */
#define PF_UNSPEC       0       /* 未指定 */
#define PF_UNIX         1       /* Unix域套接字 */
#define PF_INET         2       /* Internet IPv4 */
#define PF_INET6        10      /* Internet IPv6 */

/* 地址族定义 */
#define AF_UNSPEC       PF_UNSPEC
#define AF_UNIX         PF_UNIX
#define AF_INET         PF_INET
#define AF_INET6        PF_INET6

/* Socket类型定义 */
#define SOCK_STREAM     1       /* TCP 流式套接字 */
#define SOCK_DGRAM      2       /* UDP 数据报套接字 */
#define SOCK_RAW        3       /* 原始套接字 */

/* 协议定义 */
#define IPPROTO_IP      0       /* IP协议 */
#define IPPROTO_TCP     6       /* TCP协议 */
#define IPPROTO_UDP     17      /* UDP协议 */

/* Socket状态定义 - 模仿Linux内核的TCP状态 */
typedef enum {
    SS_UNCONNECTED = 0,         /* 未连接 */
    SS_CONNECTING,              /* 正在连接 */
    SS_CONNECTED,               /* 已连接 */
    SS_DISCONNECTING,           /* 正在断开 */
    SS_LISTENING,               /* 监听中 */
    SS_CLOSED                   /* 已关闭 */
} socket_state_t;

/* TCP连接状态 - 模仿Linux内核TCP状态机 */
typedef enum {
    TCP_ESTABLISHED = 1,        /* 已建立连接 */
    TCP_SYN_SENT,              /* 已发送SYN */
    TCP_SYN_RECV,              /* 已接收SYN */
    TCP_FIN_WAIT1,             /* 等待FIN确认 */
    TCP_FIN_WAIT2,             /* 等待对端FIN */
    TCP_TIME_WAIT,             /* TIME_WAIT状态 */
    TCP_CLOSED,                /* 关闭状态 */
    TCP_CLOSE_WAIT,            /* 等待应用层关闭 */
    TCP_LAST_ACK,              /* 等待最后ACK */
    TCP_LISTEN,                /* 监听状态 */
    TCP_CLOSING                /* 正在关闭 */
} tcp_state_t;

/* IPv4地址结构 - 模仿Linux内核 */
struct sockaddr_in {
    uint16_t sin_family;        /* 地址族 AF_INET */
    uint16_t sin_port;          /* 端口号（网络字节序） */
    uint32_t sin_addr;          /* IPv4地址（网络字节序） */
    char sin_zero[8];           /* 填充字节 */
};

/* 通用地址结构 */
struct sockaddr {
    uint16_t sa_family;         /* 地址族 */
    char sa_data[14];           /* 地址数据 */
};

/* Socket结构体 - 模仿Linux内核的socket结构 */
struct mysocket {
    int fd;                     /* 文件描述符 */
    int family;                 /* 协议族 */
    int type;                   /* Socket类型 */
    int protocol;               /* 协议 */
    socket_state_t state;       /* Socket状态 */
    tcp_state_t tcp_state;      /* TCP状态（如果是TCP） */
    
    /* 地址信息 */
    struct sockaddr_in local_addr;   /* 本地地址 */
    struct sockaddr_in peer_addr;    /* 对端地址 */
    
    /* 缓冲区 */
    char *send_buffer;          /* 发送缓冲区 */
    char *recv_buffer;          /* 接收缓冲区 */
    size_t send_buf_size;       /* 发送缓冲区大小 */
    size_t recv_buf_size;       /* 接收缓冲区大小 */
    size_t send_buf_used;       /* 发送缓冲区已使用 */
    size_t recv_buf_used;       /* 接收缓冲区已使用 */
    
    /* 监听队列（用于服务端） */
    struct mysocket **listen_queue;  /* 连接队列 */
    int listen_backlog;         /* 最大监听数量 */
    int listen_count;           /* 当前连接数量 */
    
    /* 链表指针（用于管理所有socket） */
    struct mysocket *next;
};

/* 全局Socket管理结构 */
struct socket_manager {
    struct mysocket *socket_list;    /* Socket链表头 */
    int next_fd;                     /* 下一个可用文件描述符 */
    int total_sockets;               /* 总Socket数量 */
};

/* 错误码定义 */
#define MYSOCKET_OK             0
#define MYSOCKET_ERROR          -1
#define MYSOCKET_EAGAIN         -2
#define MYSOCKET_EINVAL         -3
#define MYSOCKET_EADDRINUSE     -4
#define MYSOCKET_ECONNREFUSED   -5
#define MYSOCKET_ETIMEDOUT      -6

/* 函数声明 */

/* 初始化和清理 */
int mysocket_init(void);
void mysocket_cleanup(void);

/* 基本Socket操作 */
int mysocket_socket(int domain, int type, int protocol);
int mysocket_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int mysocket_listen(int sockfd, int backlog);
int mysocket_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int mysocket_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int mysocket_close(int sockfd);

/* 数据收发 */
ssize_t mysocket_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t mysocket_recv(int sockfd, void *buf, size_t len, int flags);
ssize_t mysocket_sendto(int sockfd, const void *buf, size_t len, int flags,
                       const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t mysocket_recvfrom(int sockfd, void *buf, size_t len, int flags,
                         struct sockaddr *src_addr, socklen_t *addrlen);

/* 辅助函数 */
const char* mysocket_strerror(int error_code);
void mysocket_print_socket_info(int sockfd);
int mysocket_set_nonblocking(int sockfd);
int mysocket_get_socket_state(int sockfd);

/* 地址转换函数 */
uint32_t mysocket_inet_addr(const char *cp);
char* mysocket_inet_ntoa(uint32_t addr);
uint16_t mysocket_htons(uint16_t hostshort);
uint16_t mysocket_ntohs(uint16_t netshort);
uint32_t mysocket_htonl(uint32_t hostlong);
uint32_t mysocket_ntohl(uint32_t netlong);

#endif /* MYSOCKET_H */