/**
 * @file server_example.c
 * @brief TCP服务器示例
 * @author Socket学习者
 * @date 2025-09-19
 */

#include "mysocket.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

int main() {
    printf("=== MySocket TCP 服务器示例 ===\n\n");
    
    /* 初始化Socket系统 */
    if (mysocket_init() != 0) {
        printf("Socket系统初始化失败\n");
        return -1;
    }
    
    /* 创建监听Socket */
    int listen_fd = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        printf("创建Socket失败: %s\n", mysocket_strerror(socket_get_error()));
        mysocket_cleanup();
        return -1;
    }
    
    printf("服务器Socket创建成功: fd=%d\n", listen_fd);
    
    /* 绑定地址 */
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = 0;  /* INADDR_ANY - 监听所有接口 */
    server_addr.sin_port = mysocket_htons(SERVER_PORT);
    
    if (mysocket_bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        printf("地址绑定失败: %s\n", mysocket_strerror(socket_get_error()));
        mysocket_close(listen_fd);
        mysocket_cleanup();
        return -1;
    }
    
    printf("地址绑定成功: 0.0.0.0:%d\n", SERVER_PORT);
    
    /* 开始监听 */
    if (mysocket_listen(listen_fd, 5) != 0) {
        printf("开始监听失败: %s\n", mysocket_strerror(socket_get_error()));
        mysocket_close(listen_fd);
        mysocket_cleanup();
        return -1;
    }
    
    printf("服务器开始监听，等待客户端连接...\n\n");
    
    /* 打印Socket信息 */
    mysocket_print_socket_info(listen_fd);
    printf("\n");
    
    /* 接受连接循环 */
    for (int i = 0; i < 3; i++) {  /* 模拟处理3个连接 */
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        printf("[%d] 等待客户端连接...\n", i + 1);
        
        int client_fd = mysocket_accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            printf("[%d] 接受连接失败: %s\n", i + 1, mysocket_strerror(socket_get_error()));
            continue;
        }
        
        printf("[%d] 客户端连接成功: fd=%d, 来自 %s:%d\n", 
               i + 1, client_fd,
               mysocket_inet_ntoa(client_addr.sin_addr),
               mysocket_ntohs(client_addr.sin_port));
        
        /* 向客户端发送欢迎信息 */
        char welcome_msg[BUFFER_SIZE];
        snprintf(welcome_msg, sizeof(welcome_msg), 
                "欢迎连接到MySocket服务器！这是第%d个连接。", i + 1);
        
        ssize_t sent = mysocket_send(client_fd, welcome_msg, strlen(welcome_msg), 0);
        if (sent > 0) {
            printf("[%d] 发送欢迎信息: %zd 字节\n", i + 1, sent);
        } else {
            printf("[%d] 发送欢迎信息失败\n", i + 1);
        }
        
        /* 尝试接收客户端数据 */
        char recv_buffer[BUFFER_SIZE];
        ssize_t received = mysocket_recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (received > 0) {
            recv_buffer[received] = '\0';
            printf("[%d] 收到客户端数据: %s\n", i + 1, recv_buffer);
            
            /* 回显数据 */
            char echo_msg[BUFFER_SIZE];
            snprintf(echo_msg, sizeof(echo_msg), "服务器回显: %s", recv_buffer);
            
            mysocket_send(client_fd, echo_msg, strlen(echo_msg), 0);
            printf("[%d] 发送回显数据\n", i + 1);
        } else {
            printf("[%d] 当前无数据可接收\n", i + 1);
        }
        
        /* 打印连接Socket信息 */
        printf("[%d] 连接Socket信息:\n", i + 1);
        mysocket_print_socket_info(client_fd);
        
        /* 关闭客户端连接 */
        mysocket_close(client_fd);
        printf("[%d] 客户端连接关闭\n\n", i + 1);
    }
    
    /* 关闭服务器Socket */
    mysocket_close(listen_fd);
    printf("服务器Socket关闭\n");
    
    /* 清理系统 */
    mysocket_cleanup();
    printf("Socket系统清理完成\n");
    
    printf("\n=== 服务器示例运行完成 ===\n");
    
    return 0;
}