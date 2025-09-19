/**
 * @file tcp_protocol.c
 * @brief 简单的TCP协议栈实现
 * @author Socket学习者
 * @date 2025-09-19
 * 
 * 实现基础的TCP状态机和数据包处理
 */

#include "socket_internal.h"

/**
 * TCP状态转换
 * @param sock Socket指针
 * @param event 触发事件
 * @return 0成功，-1失败
 */
int tcp_state_transition(struct mysocket *sock, int event) {
    if (!sock) return -1;
    
    tcp_state_t old_state = sock->tcp_state;
    
    switch (sock->tcp_state) {
        case TCP_CLOSED:
            if (event == TCP_EVENT_LISTEN) {
                sock->tcp_state = TCP_LISTEN;
            } else if (event == TCP_EVENT_CONNECT) {
                sock->tcp_state = TCP_SYN_SENT;
            }
            break;
            
        case TCP_LISTEN:
            if (event == TCP_EVENT_SYN_RECV) {
                sock->tcp_state = TCP_SYN_RECV;
            }
            break;
            
        case TCP_SYN_SENT:
            if (event == TCP_EVENT_SYN_ACK_RECV) {
                sock->tcp_state = TCP_ESTABLISHED;
            }
            break;
            
        case TCP_SYN_RECV:
            if (event == TCP_EVENT_ACK_RECV) {
                sock->tcp_state = TCP_ESTABLISHED;
            }
            break;
            
        case TCP_ESTABLISHED:
            if (event == TCP_EVENT_FIN_RECV) {
                sock->tcp_state = TCP_CLOSE_WAIT;
            } else if (event == TCP_EVENT_CLOSE) {
                sock->tcp_state = TCP_FIN_WAIT1;
            }
            break;
            
        case TCP_FIN_WAIT1:
            if (event == TCP_EVENT_ACK_RECV) {
                sock->tcp_state = TCP_FIN_WAIT2;
            } else if (event == TCP_EVENT_FIN_RECV) {
                sock->tcp_state = TCP_CLOSING;
            }
            break;
            
        case TCP_FIN_WAIT2:
            if (event == TCP_EVENT_FIN_RECV) {
                sock->tcp_state = TCP_TIME_WAIT;
            }
            break;
            
        case TCP_CLOSE_WAIT:
            if (event == TCP_EVENT_CLOSE) {
                sock->tcp_state = TCP_LAST_ACK;
            }
            break;
            
        case TCP_LAST_ACK:
            if (event == TCP_EVENT_ACK_RECV) {
                sock->tcp_state = TCP_CLOSED;
            }
            break;
            
        case TCP_CLOSING:
            if (event == TCP_EVENT_ACK_RECV) {
                sock->tcp_state = TCP_TIME_WAIT;
            }
            break;
            
        case TCP_TIME_WAIT:
            /* TIME_WAIT状态超时后转到CLOSED */
            if (event == TCP_EVENT_TIMEOUT) {
                sock->tcp_state = TCP_CLOSED;
            }
            break;
    }
    
    if (old_state != sock->tcp_state) {
        DEBUG_PRINT("TCP状态转换: fd=%d, %s -> %s", 
                    sock->fd, tcp_state_name(old_state), 
                    tcp_state_name(sock->tcp_state));
    }
    
    return 0;
}

/**
 * 获取TCP状态名称
 * @param state TCP状态
 * @return 状态名称字符串
 */
const char* tcp_state_name(tcp_state_t state) {
    switch (state) {
        case TCP_CLOSED:        return "CLOSED";
        case TCP_LISTEN:        return "LISTEN";
        case TCP_SYN_SENT:      return "SYN_SENT";
        case TCP_SYN_RECV:      return "SYN_RECV";
        case TCP_ESTABLISHED:   return "ESTABLISHED";
        case TCP_FIN_WAIT1:     return "FIN_WAIT1";
        case TCP_FIN_WAIT2:     return "FIN_WAIT2";
        case TCP_CLOSE_WAIT:    return "CLOSE_WAIT";
        case TCP_LAST_ACK:      return "LAST_ACK";
        case TCP_CLOSING:       return "CLOSING";
        case TCP_TIME_WAIT:     return "TIME_WAIT";
        default:                return "UNKNOWN";
    }
}

/**
 * 发送SYN包
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int tcp_send_syn(struct mysocket *sock) {
    if (!sock) return -1;
    
    DEBUG_PRINT("发送SYN包: fd=%d", sock->fd);
    
    /* 创建TCP包 */
    struct packet *pkt = packet_create();
    if (!pkt) return -1;
    
    /* 填充IP头 */
    pkt->ip_hdr.src_addr = sock->local_addr.sin_addr;
    pkt->ip_hdr.dst_addr = sock->peer_addr.sin_addr;
    pkt->ip_hdr.protocol = IPPROTO_TCP;
    
    /* 填充TCP头 */
    pkt->tcp_hdr.src_port = sock->local_addr.sin_port;
    pkt->tcp_hdr.dst_port = sock->peer_addr.sin_port;
    pkt->tcp_hdr.seq_num = mysocket_htonl(rand());  /* 随机初始序列号 */
    pkt->tcp_hdr.ack_num = 0;
    pkt->tcp_hdr.flags = TCP_FLAG_SYN;
    pkt->tcp_hdr.window = mysocket_htons(8192);
    
    /* 计算校验和 */
    pkt->tcp_hdr.checksum = tcp_checksum(&pkt->ip_hdr, &pkt->tcp_hdr, NULL, 0);
    
    /* 发送包 */
    int result = packet_send(pkt);
    
    /* 清理 */
    packet_destroy(pkt);
    
    if (result < 0) {
        return -1;
    }
    
    DEBUG_PRINT("SYN包发送成功: fd=%d", sock->fd);
    return 0;
}

/**
 * 发送ACK包
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int tcp_send_ack(struct mysocket *sock) {
    if (!sock) return -1;
    
    DEBUG_PRINT("发送ACK包: fd=%d", sock->fd);
    
    /* 创建TCP包 */
    struct packet *pkt = packet_create();
    if (!pkt) return -1;
    
    /* 填充IP头 */
    pkt->ip_hdr.src_addr = sock->local_addr.sin_addr;
    pkt->ip_hdr.dst_addr = sock->peer_addr.sin_addr;
    pkt->ip_hdr.protocol = IPPROTO_TCP;
    
    /* 填充TCP头 */
    pkt->tcp_hdr.src_port = sock->local_addr.sin_port;
    pkt->tcp_hdr.dst_port = sock->peer_addr.sin_port;
    pkt->tcp_hdr.seq_num = mysocket_htonl(1000);  /* 简化的序列号 */
    pkt->tcp_hdr.ack_num = mysocket_htonl(1001);  /* 简化的确认号 */
    pkt->tcp_hdr.flags = TCP_FLAG_ACK;
    pkt->tcp_hdr.window = mysocket_htons(8192);
    
    /* 计算校验和 */
    pkt->tcp_hdr.checksum = tcp_checksum(&pkt->ip_hdr, &pkt->tcp_hdr, NULL, 0);
    
    /* 发送包 */
    int result = packet_send(pkt);
    
    /* 清理 */
    packet_destroy(pkt);
    
    if (result < 0) {
        return -1;
    }
    
    DEBUG_PRINT("ACK包发送成功: fd=%d", sock->fd);
    return 0;
}

/**
 * 发送FIN包
 * @param sock Socket指针
 * @return 0成功，-1失败
 */
int tcp_send_fin(struct mysocket *sock) {
    if (!sock) return -1;
    
    DEBUG_PRINT("发送FIN包: fd=%d", sock->fd);
    
    /* 创建TCP包 */
    struct packet *pkt = packet_create();
    if (!pkt) return -1;
    
    /* 填充IP头 */
    pkt->ip_hdr.src_addr = sock->local_addr.sin_addr;
    pkt->ip_hdr.dst_addr = sock->peer_addr.sin_addr;
    pkt->ip_hdr.protocol = IPPROTO_TCP;
    
    /* 填充TCP头 */
    pkt->tcp_hdr.src_port = sock->local_addr.sin_port;
    pkt->tcp_hdr.dst_port = sock->peer_addr.sin_port;
    pkt->tcp_hdr.seq_num = mysocket_htonl(2000);  /* 简化的序列号 */
    pkt->tcp_hdr.ack_num = mysocket_htonl(2001);  /* 简化的确认号 */
    pkt->tcp_hdr.flags = TCP_FLAG_FIN;
    pkt->tcp_hdr.window = mysocket_htons(8192);
    
    /* 计算校验和 */
    pkt->tcp_hdr.checksum = tcp_checksum(&pkt->ip_hdr, &pkt->tcp_hdr, NULL, 0);
    
    /* 发送包 */
    int result = packet_send(pkt);
    
    /* 清理 */
    packet_destroy(pkt);
    
    if (result < 0) {
        return -1;
    }
    
    DEBUG_PRINT("FIN包发送成功: fd=%d", sock->fd);
    return 0;
}

/**
 * 发送TCP数据包
 * @param sock Socket指针
 * @param data 数据
 * @param len 数据长度
 * @return 0成功，-1失败
 */
int tcp_send_data(struct mysocket *sock, const void *data, size_t len) {
    if (!sock || !data || len == 0) return -1;
    
    DEBUG_PRINT("发送TCP数据: fd=%d, len=%zu", sock->fd, len);
    
    /* 创建TCP包 */
    struct packet *pkt = packet_create();
    if (!pkt) return -1;
    
    /* 分配数据空间 */
    pkt->data = malloc(len);
    if (!pkt->data) {
        packet_destroy(pkt);
        return -1;
    }
    
    memcpy(pkt->data, data, len);
    pkt->data_len = len;
    
    /* 填充IP头 */
    pkt->ip_hdr.src_addr = sock->local_addr.sin_addr;
    pkt->ip_hdr.dst_addr = sock->peer_addr.sin_addr;
    pkt->ip_hdr.protocol = IPPROTO_TCP;
    pkt->ip_hdr.total_len = mysocket_htons(sizeof(struct ip_header) + 
                                          sizeof(struct tcp_header) + len);
    
    /* 填充TCP头 */
    pkt->tcp_hdr.src_port = sock->local_addr.sin_port;
    pkt->tcp_hdr.dst_port = sock->peer_addr.sin_port;
    pkt->tcp_hdr.seq_num = mysocket_htonl(3000);  /* 简化的序列号 */
    pkt->tcp_hdr.ack_num = mysocket_htonl(3001);  /* 简化的确认号 */
    pkt->tcp_hdr.flags = TCP_FLAG_PSH | TCP_FLAG_ACK;
    pkt->tcp_hdr.window = mysocket_htons(8192);
    
    /* 计算校验和 */
    pkt->tcp_hdr.checksum = tcp_checksum(&pkt->ip_hdr, &pkt->tcp_hdr, 
                                        pkt->data, pkt->data_len);
    
    /* 发送包 */
    int result = packet_send(pkt);
    
    /* 清理 */
    packet_destroy(pkt);
    
    if (result < 0) {
        return -1;
    }
    
    DEBUG_PRINT("TCP数据发送成功: fd=%d, len=%zu", sock->fd, len);
    return 0;
}

/**
 * 处理TCP数据包
 * @param sock Socket指针
 * @param pkt 数据包
 * @return 0成功，-1失败
 */
int tcp_process_packet(struct mysocket *sock, struct packet *pkt) {
    if (!sock || !pkt) return -1;
    
    DEBUG_PRINT("处理TCP包: fd=%d, flags=0x%x", sock->fd, pkt->tcp_hdr.flags);
    
    /* 检查端口匹配 */
    if (pkt->tcp_hdr.dst_port != sock->local_addr.sin_port) {
        return -1;
    }
    
    /* 根据标志位处理 */
    if (pkt->tcp_hdr.flags & TCP_FLAG_SYN) {
        /* 处理SYN */
        if (sock->tcp_state == TCP_LISTEN) {
            tcp_state_transition(sock, TCP_EVENT_SYN_RECV);
            tcp_send_ack(sock);
        }
    }
    
    if (pkt->tcp_hdr.flags & TCP_FLAG_ACK) {
        /* 处理ACK */
        if (sock->tcp_state == TCP_SYN_SENT) {
            tcp_state_transition(sock, TCP_EVENT_SYN_ACK_RECV);
        } else if (sock->tcp_state == TCP_SYN_RECV) {
            tcp_state_transition(sock, TCP_EVENT_ACK_RECV);
        }
    }
    
    if (pkt->tcp_hdr.flags & TCP_FLAG_FIN) {
        /* 处理FIN */
        tcp_state_transition(sock, TCP_EVENT_FIN_RECV);
        tcp_send_ack(sock);
    }
    
    /* 如果有数据，写入接收缓冲区 */
    if (pkt->data_len > 0 && pkt->data) {
        size_t available = sock->recv_buf_size - sock->recv_buf_used;
        if (available > 0) {
            size_t copy_len = (pkt->data_len > available) ? available : pkt->data_len;
            memcpy(sock->recv_buffer + sock->recv_buf_used, pkt->data, copy_len);
            sock->recv_buf_used += copy_len;
            
            DEBUG_PRINT("TCP数据写入缓冲区: fd=%d, len=%zu", sock->fd, copy_len);
        }
    }
    
    return 0;
}

/**
 * 计算校验和
 * @param data 数据
 * @param len 数据长度
 * @return 校验和
 */
uint16_t checksum(void *data, size_t len) {
    uint16_t *ptr = (uint16_t *)data;
    uint32_t sum = 0;
    
    /* 16位求和 */
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    
    /* 处理奇数字节 */
    if (len == 1) {
        sum += *(uint8_t*)ptr;
    }
    
    /* 处理进位 */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

/**
 * 计算TCP校验和
 * @param ip_hdr IP头
 * @param tcp_hdr TCP头
 * @param data 数据
 * @param data_len 数据长度
 * @return TCP校验和
 */
uint16_t tcp_checksum(struct ip_header *ip_hdr, struct tcp_header *tcp_hdr, 
                     void *data, size_t data_len) {
    if (!ip_hdr || !tcp_hdr) return 0;
    
    /* 简化实现：返回固定值 */
    /* 在实际实现中需要构造伪头部并计算校验和 */
    return 0x1234;
}