/**
 * @file socket_utils.c
 * @brief Socket 辅助工具函数
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 实现数据包处理、地址转换等辅助功能
 */

#include "socket_internal.h"

/* TCP事件定义 */
#define TCP_EVENT_LISTEN        1
#define TCP_EVENT_CONNECT       2
#define TCP_EVENT_SYN_RECV      3
#define TCP_EVENT_SYN_ACK_RECV  4
#define TCP_EVENT_ACK_RECV      5
#define TCP_EVENT_FIN_RECV      6
#define TCP_EVENT_CLOSE         7
#define TCP_EVENT_TIMEOUT       8

/**
 * 创建数据包
 * @return 数据包指针，失败返回NULL
 */
struct packet* packet_create(void) {
    struct packet *pkt = calloc(1, sizeof(struct packet));
    if (!pkt) return NULL;
    
    /* 初始化包头 */
    memset(&pkt->ip_hdr, 0, sizeof(pkt->ip_hdr));
    memset(&pkt->tcp_hdr, 0, sizeof(pkt->tcp_hdr));
    
    pkt->data = NULL;
    pkt->data_len = 0;
    pkt->next = NULL;
    
    return pkt;
}

/**
 * 销毁数据包
 * @param pkt 数据包指针
 */
void packet_destroy(struct packet *pkt) {
    if (!pkt) return;
    
    if (pkt->data) {
        free(pkt->data);
    }
    
    free(pkt);
}

/**
 * 发送数据包
 * @param pkt 数据包
 * @return 0成功，-1失败
 */
int packet_send(struct packet *pkt) {
    if (!pkt) return -1;
    
    DEBUG_PRINT("发送数据包: src=%08x:%d -> dst=%08x:%d", 
                pkt->ip_hdr.src_addr, mysocket_ntohs(pkt->tcp_hdr.src_port),
                pkt->ip_hdr.dst_addr, mysocket_ntohs(pkt->tcp_hdr.dst_port));
    
    /* 模拟数据包发送 */
    /* 在实际实现中，这里会通过网络接口发送数据包 */
    
    /* 简单模拟：查找目标Socket并投递数据包 */
    struct mysocket_addr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr = pkt->ip_hdr.dst_addr;
    target_addr.sin_port = pkt->tcp_hdr.dst_port;
    
    struct mysocket *target = socket_find_by_address(&target_addr);
    if (target) {
        /* 处理数据包 */
        if (pkt->ip_hdr.protocol == IPPROTO_TCP) {
            tcp_process_packet(target, pkt);
        }
        return 0;
    }
    
    /* 目标不存在，模拟网络丢包 */
    DEBUG_PRINT("数据包投递失败: 目标不存在");
    return -1;
}

/**
 * 接收数据包
 * @param sock Socket指针
 * @return 数据包指针，无数据返回NULL
 */
struct packet* packet_receive(struct mysocket *sock) {
    if (!sock) return NULL;
    
    /* 简化实现：模拟从网络接收数据包 */
    /* 在实际实现中，这里会从网络接口读取数据包 */
    
    return NULL;  /* 暂不实现 */
}

/**
 * 根据地址查找Socket
 * @param addr 地址
 * @return Socket指针，未找到返回NULL
 */
struct mysocket* socket_find_by_address(const struct mysocket_addr_in *addr) {
    if (!addr) return NULL;
    
    struct mysocket *current = g_socket_manager.socket_list;
    
    while (current != NULL) {
        /* 检查地址匹配 */
        if (current->local_addr.sin_port == addr->sin_port) {
            /* 检查IP匹配（通配或精确） */
            if (current->local_addr.sin_addr == 0 || 
                current->local_addr.sin_addr == addr->sin_addr) {
                return current;
            }
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * 网络字节序转换：主机到网络（16位）
 */
uint16_t mysocket_htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

/**
 * 网络字节序转换：网络到主机（16位）
 */
uint16_t mysocket_ntohs(uint16_t netshort) {
    return ((netshort & 0xFF) << 8) | ((netshort >> 8) & 0xFF);
}

/**
 * 网络字节序转换：主机到网络（32位）
 */
uint32_t mysocket_htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) | 
           (((hostlong >> 8) & 0xFF) << 16) |
           (((hostlong >> 16) & 0xFF) << 8) |
           ((hostlong >> 24) & 0xFF);
}

/**
 * 网络字节序转换：网络到主机（32位）
 */
uint32_t mysocket_ntohl(uint32_t netlong) {
    return ((netlong & 0xFF) << 24) | 
           (((netlong >> 8) & 0xFF) << 16) |
           (((netlong >> 16) & 0xFF) << 8) |
           ((netlong >> 24) & 0xFF);
}

/**
 * 字符串IP地址转换为网络地址
 */
uint32_t mysocket_inet_addr(const char *cp) {
    if (!cp) return 0;
    
    unsigned int a, b, c, d;
    if (sscanf(cp, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return 0;
    }
    
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        return 0;
    }
    
    return mysocket_htonl((a << 24) | (b << 16) | (c << 8) | d);
}

/**
 * 网络地址转换为字符串IP地址
 */
char* mysocket_inet_ntoa(uint32_t addr) {
    static char str[16];
    uint32_t host_addr = mysocket_ntohl(addr);
    
    snprintf(str, sizeof(str), "%u.%u.%u.%u",
             (host_addr >> 24) & 0xFF,
             (host_addr >> 16) & 0xFF,
             (host_addr >> 8) & 0xFF,
             host_addr & 0xFF);
    
    return str;
}

/**
 * 设置Socket为非阻塞模式
 */
int mysocket_set_nonblocking(int sockfd) {
    struct mysocket *sock = socket_find_by_fd(sockfd);
    if (!sock) {
        socket_set_error(MYSOCKET_EINVAL);
        return -1;
    }
    
    /* 简单实现：设置标志位 */
    /* 在实际实现中需要修改Socket的阻塞属性 */
    
    DEBUG_PRINT("Socket设置为非阻塞: fd=%d", sockfd);
    return 0;
}

/**
 * 获取当前时间戳
 */
uint32_t get_current_timestamp(void) {
    return (uint32_t)time(NULL);
}

/**
 * 打印Socket调试信息
 */
void socket_print_debug_info(struct mysocket *sock, const char *msg) {
    if (!sock || !msg) return;
    
    DEBUG_PRINT("%s - Socket fd=%d:", msg, sock->fd);
    DEBUG_PRINT("  状态: %d, TCP状态: %s", sock->state, tcp_state_name(sock->tcp_state));
    DEBUG_PRINT("  本地: %s:%d", 
                mysocket_inet_ntoa(sock->local_addr.sin_addr),
                mysocket_ntohs(sock->local_addr.sin_port));
    DEBUG_PRINT("  对端: %s:%d", 
                mysocket_inet_ntoa(sock->peer_addr.sin_addr),
                mysocket_ntohs(sock->peer_addr.sin_port));
    DEBUG_PRINT("  缓冲区: send=%zu/%zu, recv=%zu/%zu",
                sock->send_buf_used, sock->send_buf_size,
                sock->recv_buf_used, sock->recv_buf_size);
}

/**
 * 创建地址结构
 */
struct mysocket_addr_in mysocket_make_addr(const char *ip, uint16_t port) {
    struct mysocket_addr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family = AF_INET;
    addr.sin_port = mysocket_htons(port);
    
    if (ip == NULL || strcmp(ip, "0.0.0.0") == 0) {
        addr.sin_addr = 0;  /* INADDR_ANY */
    } else {
        addr.sin_addr = mysocket_inet_addr(ip);
    }
    
    return addr;
}

/**
 * 检查地址是否有效
 */
int mysocket_addr_is_valid(const struct mysocket_addr_in *addr) {
    if (!addr) return 0;
    
    return (addr->sin_family == AF_INET) && (addr->sin_port != 0);
}

/**
 * 比较两个地址
 */
int mysocket_addr_equal(const struct mysocket_addr_in *addr1, 
                       const struct mysocket_addr_in *addr2) {
    if (!addr1 || !addr2) return 0;
    
    return (addr1->sin_family == addr2->sin_family) &&
           (addr1->sin_addr == addr2->sin_addr) &&
           (addr1->sin_port == addr2->sin_port);
}

/**
 * 生成随机端口号
 */
uint16_t mysocket_random_port(void) {
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    
    /* 临时端口范围 49152-65535 */
    return 49152 + (rand() % (65535 - 49152 + 1));
}

/**
 * 检查端口是否在使用中
 */
int mysocket_port_in_use(uint16_t port) {
    struct mysocket *current = g_socket_manager.socket_list;
    
    while (current != NULL) {
        if (mysocket_ntohs(current->local_addr.sin_port) == port) {
            return 1;
        }
        current = current->next;
    }
    
    return 0;
}

/**
 * 格式化地址为字符串
 */
void mysocket_addr_to_string(const struct mysocket_addr_in *addr, char *buf, size_t len) {
    if (!addr || !buf || len == 0) return;
    
    snprintf(buf, len, "%s:%u", 
             mysocket_inet_ntoa(addr->sin_addr),
             mysocket_ntohs(addr->sin_port));
}