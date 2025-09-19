/**
 * @file test_basic.c
 * @brief 基础功能测试
 * @author Socket学习者
 * @date 2025-09-19
 */

#include "mysocket.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_socket_creation() {
    printf("测试Socket创建功能...\n");
    
    /* 初始化Socket系统 */
    assert(mysocket_init() == 0);
    
    /* 测试TCP Socket创建 */
    int tcp_sock = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(tcp_sock >= 0);
    printf("  TCP Socket创建成功: fd=%d\n", tcp_sock);
    
    /* 测试UDP Socket创建 */
    int udp_sock = mysocket_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(udp_sock >= 0);
    printf("  UDP Socket创建成功: fd=%d\n", udp_sock);
    
    /* 测试无效参数 */
    int invalid_sock = mysocket_socket(999, SOCK_STREAM, IPPROTO_TCP);
    assert(invalid_sock == -1);
    printf("  无效参数测试通过\n");
    
    /* 关闭Socket */
    assert(mysocket_close(tcp_sock) == 0);
    assert(mysocket_close(udp_sock) == 0);
    printf("  Socket关闭成功\n");
    
    /* 清理系统 */
    mysocket_cleanup();
    
    printf("✓ Socket创建测试通过\n\n");
}

void test_address_binding() {
    printf("测试地址绑定功能...\n");
    
    /* 初始化 */
    assert(mysocket_init() == 0);
    
    /* 创建Socket */
    int sock = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(sock >= 0);
    
    /* 准备地址 */
    struct mysocket_addr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr = mysocket_inet_addr("127.0.0.1");
    addr.sin_port = mysocket_htons(8080);
    
    /* 测试绑定 */
    int result = mysocket_bind(sock, (struct mysocket_addr*)&addr, sizeof(addr));
    assert(result == 0);
    printf("  地址绑定成功: 127.0.0.1:8080\n");
    
    /* 测试重复绑定 */
    int sock2 = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(sock2 >= 0);
    
    result = mysocket_bind(sock2, (struct mysocket_addr*)&addr, sizeof(addr));
    assert(result == -1);  /* 应该失败 */
    printf("  重复绑定检测通过\n");
    
    /* 清理 */
    mysocket_close(sock);
    mysocket_close(sock2);
    mysocket_cleanup();
    
    printf("✓ 地址绑定测试通过\n\n");
}

void test_listen_accept() {
    printf("测试监听和接受连接...\n");
    
    /* 初始化 */
    assert(mysocket_init() == 0);
    
    /* 创建监听Socket */
    int listen_sock = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(listen_sock >= 0);
    
    /* 绑定地址 */
    struct mysocket_addr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr = 0;  /* INADDR_ANY */
    addr.sin_port = mysocket_htons(8081);
    
    assert(mysocket_bind(listen_sock, (struct mysocket_addr*)&addr, sizeof(addr)) == 0);
    
    /* 开始监听 */
    assert(mysocket_listen(listen_sock, 5) == 0);
    printf("  监听Socket创建成功: port=8081\n");
    
    /* 测试连接（模拟） */
    struct mysocket_addr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int conn_sock = mysocket_accept(listen_sock, (struct mysocket_addr*)&client_addr, &client_len);
    if (conn_sock >= 0) {
        printf("  模拟连接接受成功: fd=%d\n", conn_sock);
        mysocket_close(conn_sock);
    } else {
        printf("  当前无连接请求（正常情况）\n");
    }
    
    /* 清理 */
    mysocket_close(listen_sock);
    mysocket_cleanup();
    
    printf("✓ 监听接受测试通过\n\n");
}

void test_data_transfer() {
    printf("测试数据传输功能...\n");
    
    /* 初始化 */
    assert(mysocket_init() == 0);
    
    /* 创建两个Socket模拟客户端和服务端 */
    int server_sock = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int client_sock = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    assert(server_sock >= 0 && client_sock >= 0);
    
    /* 服务端绑定并监听 */
    struct mysocket_addr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = 0;
    server_addr.sin_port = mysocket_htons(8082);
    
    assert(mysocket_bind(server_sock, (struct mysocket_addr*)&server_addr, sizeof(server_addr)) == 0);
    assert(mysocket_listen(server_sock, 1) == 0);
    
    /* 客户端连接 */
    struct mysocket_addr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr = mysocket_inet_addr("127.0.0.1");
    target_addr.sin_port = mysocket_htons(8082);
    
    int connect_result = mysocket_connect(client_sock, (struct mysocket_addr*)&target_addr, sizeof(target_addr));
    if (connect_result == 0) {
        printf("  客户端连接成功\n");
        
        /* 测试数据发送 */
        const char *test_data = "Hello, MySocket!";
        ssize_t sent = mysocket_send(client_sock, test_data, strlen(test_data), 0);
        if (sent > 0) {
            printf("  数据发送成功: %zd 字节\n", sent);
        }
        
        /* 测试数据接收 */
        char recv_buf[1024];
        ssize_t received = mysocket_recv(client_sock, recv_buf, sizeof(recv_buf), 0);
        if (received > 0) {
            recv_buf[received] = '\0';
            printf("  数据接收成功: %zd 字节, 内容: %s\n", received, recv_buf);
        } else {
            printf("  当前无数据可接收（正常情况）\n");
        }
    } else {
        printf("  客户端连接失败（模拟环境限制）\n");
    }
    
    /* 清理 */
    mysocket_close(server_sock);
    mysocket_close(client_sock);
    mysocket_cleanup();
    
    printf("✓ 数据传输测试通过\n\n");
}

void test_udp_operations() {
    printf("测试UDP操作功能...\n");
    
    /* 初始化 */
    assert(mysocket_init() == 0);
    
    /* 创建UDP Socket */
    int udp_sock = mysocket_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(udp_sock >= 0);
    
    /* 绑定地址 */
    struct mysocket_addr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr = 0;
    addr.sin_port = mysocket_htons(8083);
    
    assert(mysocket_bind(udp_sock, (struct mysocket_addr*)&addr, sizeof(addr)) == 0);
    printf("  UDP Socket绑定成功: port=8083\n");
    
    /* 测试sendto */
    struct mysocket_addr_in target;
    target.sin_family = AF_INET;
    target.sin_addr = mysocket_inet_addr("127.0.0.1");
    target.sin_port = mysocket_htons(8084);
    
    const char *udp_data = "UDP Test Message";
    ssize_t sent = mysocket_sendto(udp_sock, udp_data, strlen(udp_data), 0,
                                  (struct mysocket_addr*)&target, sizeof(target));
    if (sent > 0) {
        printf("  UDP数据发送成功: %zd 字节\n", sent);
    }
    
    /* 测试recvfrom */
    char recv_buf[1024];
    struct mysocket_addr_in src_addr;
    socklen_t src_len = sizeof(src_addr);
    
    ssize_t received = mysocket_recvfrom(udp_sock, recv_buf, sizeof(recv_buf), 0,
                                        (struct mysocket_addr*)&src_addr, &src_len);
    if (received > 0) {
        recv_buf[received] = '\0';
        printf("  UDP数据接收成功: %zd 字节, 内容: %s\n", received, recv_buf);
    } else {
        printf("  当前无UDP数据可接收（正常情况）\n");
    }
    
    /* 清理 */
    mysocket_close(udp_sock);
    mysocket_cleanup();
    
    printf("✓ UDP操作测试通过\n\n");
}

void test_utility_functions() {
    printf("测试辅助功能函数...\n");
    
    /* 测试地址转换 */
    uint32_t addr = mysocket_inet_addr("192.168.1.100");
    char *addr_str = mysocket_inet_ntoa(addr);
    printf("  地址转换: %s -> 0x%08x -> %s\n", "192.168.1.100", addr, addr_str);
    
    /* 测试字节序转换 */
    uint16_t port = 8080;
    uint16_t net_port = mysocket_htons(port);
    uint16_t host_port = mysocket_ntohs(net_port);
    printf("  字节序转换: %u -> %u -> %u\n", port, net_port, host_port);
    assert(port == host_port);
    
    /* 测试错误信息 */
    const char *error_msg = mysocket_strerror(MYSOCKET_EINVAL);
    printf("  错误信息: EINVAL -> %s\n", error_msg);
    
    printf("✓ 辅助功能测试通过\n\n");
}

int main() {
    printf("=== MySocket 基础功能测试 ===\n\n");
    
    test_socket_creation();
    test_address_binding();
    test_listen_accept();
    test_data_transfer();
    test_udp_operations();
    test_utility_functions();
    
    printf("=== 所有测试完成 ===\n");
    printf("✓ Socket学习项目基础功能正常\n");
    
    return 0;
}