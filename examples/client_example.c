/**
 * @file client_example.c
 * @brief TCP客户端示例
 * @author Socket学习者
 * @date 2025-09-19
 */

#include "mysocket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

int main() {
    printf("=== MySocket TCP 客户端示例 ===\n\n");
    
    /* 初始化Socket系统 */
    if (mysocket_init() != 0) {
        printf("Socket系统初始化失败\n");
        return -1;
    }
    
    /* 创建客户端Socket */
    int client_fd = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_fd < 0) {
        printf("创建Socket失败: %s\n", mysocket_strerror(socket_get_error()));
        mysocket_cleanup();
        return -1;
    }
    
    printf("客户端Socket创建成功: fd=%d\n", client_fd);
    
    /* 准备服务器地址 */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = mysocket_inet_addr(SERVER_IP);
    server_addr.sin_port = mysocket_htons(SERVER_PORT);
    
    printf("准备连接到服务器: %s:%d\n", SERVER_IP, SERVER_PORT);
    
    /* 连接到服务器 */
    if (mysocket_connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        printf("连接服务器失败: %s\n", mysocket_strerror(socket_get_error()));
        mysocket_close(client_fd);
        mysocket_cleanup();
        return -1;
    }
    
    printf("成功连接到服务器！\n\n");
    
    /* 打印Socket信息 */
    printf("客户端Socket信息:\n");
    mysocket_print_socket_info(client_fd);
    printf("\n");
    
    /* 接收服务器欢迎信息 */
    char recv_buffer[BUFFER_SIZE];
    printf("等待服务器欢迎信息...\n");
    
    ssize_t received = mysocket_recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (received > 0) {
        recv_buffer[received] = '\0';
        printf("收到服务器信息: %s\n", recv_buffer);
    } else {
        printf("未收到服务器信息\n");
    }
    
    /* 发送测试数据 */
    const char *test_messages[] = {
        "Hello, MySocket Server!",
        "这是一个测试消息",
        "Socket学习项目运行正常",
        "再见服务器！"
    };
    
    int num_messages = sizeof(test_messages) / sizeof(test_messages[0]);
    
    for (int i = 0; i < num_messages; i++) {
        printf("\n[消息 %d] 发送: %s\n", i + 1, test_messages[i]);
        
        /* 发送消息 */
        ssize_t sent = mysocket_send(client_fd, test_messages[i], 
                                    strlen(test_messages[i]), 0);
        if (sent > 0) {
            printf("[消息 %d] 发送成功: %zd 字节\n", i + 1, sent);
        } else {
            printf("[消息 %d] 发送失败: %s\n", i + 1, 
                   mysocket_strerror(socket_get_error()));
            continue;
        }
        
        /* 接收服务器回显 */
        received = mysocket_recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (received > 0) {
            recv_buffer[received] = '\0';
            printf("[消息 %d] 收到回显: %s\n", i + 1, recv_buffer);
        } else {
            printf("[消息 %d] 未收到回显\n", i + 1);
        }
        
        /* 模拟处理延迟 */
        printf("[消息 %d] 处理完成\n", i + 1);
    }
    
    /* 关闭连接 */
    printf("\n关闭客户端连接...\n");
    mysocket_close(client_fd);
    printf("客户端Socket关闭\n");
    
    /* 清理系统 */
    mysocket_cleanup();
    printf("Socket系统清理完成\n");
    
    printf("\n=== 客户端示例运行完成 ===\n");
    
    return 0;
}