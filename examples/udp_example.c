/**
 * @file udp_example.c
 * @brief UDP 通信示例
 * @author Socket学习者
 * @date 2025-09-19
 */

#include "mysocket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define UDP_PORT_A 9001
#define UDP_PORT_B 9002
#define BUFFER_SIZE 1024

void test_udp_communication() {
    printf("=== MySocket UDP 通信示例 ===\n\n");
    
    /* 初始化Socket系统 */
    if (mysocket_init() != 0) {
        printf("Socket系统初始化失败\n");
        return;
    }
    
    /* 创建两个UDP Socket */
    int udp_a = mysocket_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int udp_b = mysocket_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (udp_a < 0 || udp_b < 0) {
        printf("创建UDP Socket失败\n");
        mysocket_cleanup();
        return;
    }
    
    printf("UDP Socket创建成功: A=%d, B=%d\n", udp_a, udp_b);
    
    /* 绑定地址 */
    struct mysocket_addr_in addr_a, addr_b;
    
    /* Socket A 绑定到端口 9001 */
    addr_a.sin_family = AF_INET;
    addr_a.sin_addr = mysocket_inet_addr("127.0.0.1");
    addr_a.sin_port = mysocket_htons(UDP_PORT_A);
    
    /* Socket B 绑定到端口 9002 */
    addr_b.sin_family = AF_INET;
    addr_b.sin_addr = mysocket_inet_addr("127.0.0.1");
    addr_b.sin_port = mysocket_htons(UDP_PORT_B);
    
    if (mysocket_bind(udp_a, (struct mysocket_addr*)&addr_a, sizeof(addr_a)) != 0) {
        printf("绑定Socket A失败\n");
        goto cleanup;
    }
    
    if (mysocket_bind(udp_b, (struct mysocket_addr*)&addr_b, sizeof(addr_b)) != 0) {
        printf("绑定Socket B失败\n");
        goto cleanup;
    }
    
    printf("地址绑定成功: A=127.0.0.1:%d, B=127.0.0.1:%d\n\n", UDP_PORT_A, UDP_PORT_B);
    
    /* 打印Socket信息 */
    printf("Socket A 信息:\n");
    mysocket_print_socket_info(udp_a);
    printf("\nSocket B 信息:\n");
    mysocket_print_socket_info(udp_b);
    printf("\n");
    
    /* 测试 A -> B 通信 */
    printf("=== 测试 A -> B 通信 ===\n");
    
    const char *msg_a_to_b = "Hello from Socket A to Socket B!";
    printf("A 发送消息: %s\n", msg_a_to_b);
    
    ssize_t sent = mysocket_sendto(udp_a, msg_a_to_b, strlen(msg_a_to_b), 0,
                                  (struct mysocket_addr*)&addr_b, sizeof(addr_b));
    if (sent > 0) {
        printf("A 发送成功: %zd 字节\n", sent);
    } else {
        printf("A 发送失败\n");
    }
    
    /* B 尝试接收 */
    char recv_buffer[BUFFER_SIZE];
    struct mysocket_addr_in src_addr;
    socklen_t src_len = sizeof(src_addr);
    
    ssize_t received = mysocket_recvfrom(udp_b, recv_buffer, sizeof(recv_buffer) - 1, 0,
                                        (struct mysocket_addr*)&src_addr, &src_len);
    if (received > 0) {
        recv_buffer[received] = '\0';
        printf("B 收到消息: %s\n", recv_buffer);
        printf("B 消息来源: %s:%d\n", 
               mysocket_inet_ntoa(src_addr.sin_addr),
               mysocket_ntohs(src_addr.sin_port));
    } else {
        printf("B 未收到消息\n");
    }
    
    printf("\n");
    
    /* 测试 B -> A 通信 */
    printf("=== 测试 B -> A 通信 ===\n");
    
    const char *msg_b_to_a = "Hello back from Socket B to Socket A!";
    printf("B 发送消息: %s\n", msg_b_to_a);
    
    sent = mysocket_sendto(udp_b, msg_b_to_a, strlen(msg_b_to_a), 0,
                          (struct mysocket_addr*)&addr_a, sizeof(addr_a));
    if (sent > 0) {
        printf("B 发送成功: %zd 字节\n", sent);
    } else {
        printf("B 发送失败\n");
    }
    
    /* A 尝试接收 */
    received = mysocket_recvfrom(udp_a, recv_buffer, sizeof(recv_buffer) - 1, 0,
                                (struct mysocket_addr*)&src_addr, &src_len);
    if (received > 0) {
        recv_buffer[received] = '\0';
        printf("A 收到消息: %s\n", recv_buffer);
        printf("A 消息来源: %s:%d\n",
               mysocket_inet_ntoa(src_addr.sin_addr),
               mysocket_ntohs(src_addr.sin_port));
    } else {
        printf("A 未收到消息\n");
    }
    
    printf("\n");
    
    /* 测试广播式发送 */
    printf("=== 测试多消息发送 ===\n");
    
    const char *messages[] = {
        "UDP消息 1: 测试数据传输",
        "UDP消息 2: Socket学习项目",
        "UDP消息 3: 网络编程实践",
        "UDP消息 4: 底层原理学习"
    };
    
    int num_messages = sizeof(messages) / sizeof(messages[0]);
    
    for (int i = 0; i < num_messages; i++) {
        printf("[%d] A 发送: %s\n", i + 1, messages[i]);
        
        sent = mysocket_sendto(udp_a, messages[i], strlen(messages[i]), 0,
                              (struct mysocket_addr*)&addr_b, sizeof(addr_b));
        if (sent > 0) {
            printf("[%d] 发送成功: %zd 字节\n", i + 1, sent);
        }
        
        /* 尝试接收 */
        received = mysocket_recvfrom(udp_b, recv_buffer, sizeof(recv_buffer) - 1, 0,
                                    (struct mysocket_addr*)&src_addr, &src_len);
        if (received > 0) {
            recv_buffer[received] = '\0';
            printf("[%d] B 收到: %s\n", i + 1, recv_buffer);
        } else {
            printf("[%d] B 未收到数据\n", i + 1);
        }
        
        printf("\n");
    }

cleanup:
    /* 清理资源 */
    mysocket_close(udp_a);
    mysocket_close(udp_b);
    mysocket_cleanup();
    
    printf("=== UDP 通信示例完成 ===\n");
}

int main() {
    test_udp_communication();
    return 0;
}