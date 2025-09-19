/**
 * @file socket_internal.h
 * @brief Socket 内部实现头文件
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 包含内部数据结构和函数声明，不对外暴露
 */

#ifndef SOCKET_INTERNAL_H
#define SOCKET_INTERNAL_H

#include "mysocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/* 调试宏定义 */
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    printf("[DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) do {} while(0)
#endif

/* 缓冲区默认大小 */
#define DEFAULT_SEND_BUFFER_SIZE    8192
#define DEFAULT_RECV_BUFFER_SIZE    8192
#define DEFAULT_LISTEN_BACKLOG      128

/* TCP包头结构（简化版） */
struct tcp_header {
    uint16_t src_port;          /* 源端口 */
    uint16_t dst_port;          /* 目标端口 */
    uint32_t seq_num;           /* 序列号 */
    uint32_t ack_num;           /* 确认号 */
    uint16_t flags;             /* 标志位 */
    uint16_t window;            /* 窗口大小 */
    uint16_t checksum;          /* 校验和 */
    uint16_t urgent;            /* 紧急指针 */
};

/* TCP标志位定义 */
#define TCP_FLAG_FIN    0x01
#define TCP_FLAG_SYN    0x02
#define TCP_FLAG_RST    0x04
#define TCP_FLAG_PSH    0x08
#define TCP_FLAG_ACK    0x10
#define TCP_FLAG_URG    0x20

/* IP包头结构（简化版） */
struct ip_header {
    uint8_t version_ihl;        /* 版本和头长度 */
    uint8_t tos;                /* 服务类型 */
    uint16_t total_len;         /* 总长度 */
    uint16_t id;                /* 标识 */
    uint16_t flags_frag;        /* 标志和分片偏移 */
    uint8_t ttl;                /* 生存时间 */
    uint8_t protocol;           /* 协议 */
    uint16_t checksum;          /* 头校验和 */
    uint32_t src_addr;          /* 源地址 */
    uint32_t dst_addr;          /* 目标地址 */
};

/* 数据包结构 */
struct packet {
    struct ip_header ip_hdr;
    struct tcp_header tcp_hdr;
    char *data;
    size_t data_len;
    struct packet *next;        /* 链表指针 */
};

/* 连接控制块（类似Linux内核的sock结构） */
struct connection_cb {
    struct mysocket *sock;      /* 关联的socket */
    uint32_t snd_una;          /* 发送未确认序列号 */
    uint32_t snd_nxt;          /* 发送下一个序列号 */
    uint32_t snd_wnd;          /* 发送窗口 */
    uint32_t rcv_nxt;          /* 接收下一个序列号 */
    uint32_t rcv_wnd;          /* 接收窗口 */
    
    /* 重传机制 */
    struct packet *retrans_queue; /* 重传队列 */
    time_t last_ack_time;       /* 最后ACK时间 */
    int retrans_count;          /* 重传次数 */
};

/* 全局变量声明 */
extern struct socket_manager g_socket_manager;

/* 内部函数声明 */

/* Socket管理 */
struct mysocket* socket_find_by_fd(int fd);
struct mysocket* socket_create(int domain, int type, int protocol);
void socket_destroy(struct mysocket *sock);
int socket_add_to_manager(struct mysocket *sock);
void socket_remove_from_manager(struct mysocket *sock);

/* 地址处理 */
int socket_addr_copy(struct mysocket_addr_in *dst, const struct mysocket_addr *src, socklen_t addrlen);
int socket_addr_compare(const struct mysocket_addr_in *addr1, const struct mysocket_addr_in *addr2);
int socket_addr_is_any(const struct mysocket_addr_in *addr);

/* 缓冲区管理 */
int socket_buffer_init(struct mysocket *sock);
void socket_buffer_cleanup(struct mysocket *sock);
int socket_buffer_write(char *buffer, size_t *used, size_t total, 
                       const void *data, size_t len);
int socket_buffer_read(char *buffer, size_t *used, void *data, size_t len);

/* TCP状态机 */
int tcp_state_transition(struct mysocket *sock, int event);
const char* tcp_state_name(tcp_state_t state);

/* 协议处理 */
int tcp_process_packet(struct mysocket *sock, struct packet *pkt);
int tcp_send_syn(struct mysocket *sock);
int tcp_send_ack(struct mysocket *sock);
int tcp_send_fin(struct mysocket *sock);
int tcp_send_data(struct mysocket *sock, const void *data, size_t len);

/* 数据包处理 */
struct packet* packet_create(void);
void packet_destroy(struct packet *pkt);
int packet_send(struct packet *pkt);
struct packet* packet_receive(struct mysocket *sock);

/* 校验和计算 */
uint16_t checksum(void *data, size_t len);
uint16_t tcp_checksum(struct ip_header *ip_hdr, struct tcp_header *tcp_hdr, 
                     void *data, size_t data_len);

/* TCP事件定义 */
#define TCP_EVENT_LISTEN        1
#define TCP_EVENT_CONNECT       2
#define TCP_EVENT_SYN_RECV      3
#define TCP_EVENT_SYN_ACK_RECV  4
#define TCP_EVENT_ACK_RECV      5
#define TCP_EVENT_FIN_RECV      6
#define TCP_EVENT_CLOSE         7
#define TCP_EVENT_TIMEOUT       8

/* 地址处理扩展 */
int socket_check_addr_in_use(const struct mysocket_addr_in *addr, int exclude_fd);
int socket_listen_queue_add(struct mysocket *listen_sock, struct mysocket *new_sock);
struct mysocket* socket_listen_queue_remove(struct mysocket *listen_sock);
int socket_listen_queue_status(struct mysocket *listen_sock, int *count, int *backlog);

/* 连接处理 */
int socket_auto_bind(struct mysocket *sock);
int socket_simulate_tcp_handshake(struct mysocket *sock);
struct mysocket* socket_simulate_incoming_connection(struct mysocket *listen_sock);
struct mysocket* socket_find_listening_socket(const struct mysocket_addr_in *addr);
int socket_can_accept_connection(struct mysocket *listen_sock, const struct mysocket_addr_in *peer_addr);

/* 缓冲区扩展功能 */
int socket_buffer_resize(struct mysocket *sock, size_t send_size, size_t recv_size);
void socket_buffer_clear(struct mysocket *sock, int clear_send, int clear_recv);
int socket_buffer_status(struct mysocket *sock, size_t *send_used, size_t *send_free,
                        size_t *recv_used, size_t *recv_free);
int socket_buffer_has_space(struct mysocket *sock, size_t send_need, size_t recv_need);

/* 数据传输 */
int socket_flush_send_buffer(struct mysocket *sock);
int socket_fill_recv_buffer(struct mysocket *sock);
ssize_t socket_send_udp_packet(struct mysocket *sock, const void *data, size_t len);
ssize_t socket_recv_udp_packet(struct mysocket *sock, void *buf, size_t len,
                              struct mysocket_addr_in *src_addr);
struct mysocket* socket_find_udp_receiver(const struct mysocket_addr_in *addr);
size_t socket_simulate_tcp_receive(struct mysocket *sock, void *buf, size_t len);

/* 地址查找和管理 */
struct mysocket* socket_find_by_address(const struct mysocket_addr_in *addr);

/* 辅助工具函数 */
struct mysocket_addr_in mysocket_make_addr(const char *ip, uint16_t port);
int mysocket_addr_is_valid(const struct mysocket_addr_in *addr);
int mysocket_addr_equal(const struct mysocket_addr_in *addr1, const struct mysocket_addr_in *addr2);
uint16_t mysocket_random_port(void);
int mysocket_port_in_use(uint16_t port);
void mysocket_addr_to_string(const struct mysocket_addr_in *addr, char *buf, size_t len);

/* 辅助工具 */
void socket_print_debug_info(struct mysocket *sock, const char *msg);
uint32_t get_current_timestamp(void);
void socket_set_error(int error_code);
int socket_get_error(void);

#endif /* SOCKET_INTERNAL_H */