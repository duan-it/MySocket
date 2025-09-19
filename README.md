# MySocket - Socket 底层实现学习项目

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C](https://img.shields.io/badge/language-C-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey.svg)

## 项目简介

MySocket 是一个用 C 语言编写的 Socket 套接字学习项目，模仿 Linux 内核的实现方式，旨在帮助理解 Socket 的底层原理和网络编程的核心概念。

### 主要特性

- ✅ **完整的 Socket API 实现**：包含 socket、bind、listen、accept、connect、send、recv 等核心函数
- ✅ **TCP/UDP 协议支持**：实现了基础的 TCP 状态机和 UDP 数据报传输
- ✅ **内存管理**：自动管理 Socket 生命周期和缓冲区
- ✅ **错误处理**：完善的错误码和错误信息系统
- ✅ **线程安全**：使用互斥锁保护关键数据结构
- ✅ **调试支持**：内置调试信息输出和状态查看功能

## 项目结构

```
mysocket/
├── include/                 # 头文件目录
│   ├── mysocket.h          # 公共接口头文件
│   └── socket_internal.h   # 内部实现头文件
├── src/                    # 源代码目录
│   ├── socket_core.c       # Socket 核心功能
│   ├── socket_buffer.c     # 缓冲区管理
│   ├── socket_bind_listen.c # bind 和 listen 实现
│   ├── socket_accept_connect.c # accept 和 connect 实现
│   ├── socket_sendrecv.c   # 数据收发实现
│   ├── tcp_protocol.c      # TCP 协议栈
│   └── socket_utils.c      # 辅助工具函数
├── tests/                  # 测试程序
│   └── test_basic.c        # 基础功能测试
├── examples/               # 示例程序
│   ├── server_example.c    # TCP 服务器示例
│   ├── client_example.c    # TCP 客户端示例
│   └── udp_example.c       # UDP 通信示例
├── obj/                    # 编译对象文件（编译时生成）
├── bin/                    # 可执行文件（编译时生成）
├── Makefile               # 构建配置
└── README.md              # 项目文档
```

## 快速开始

### 编译环境要求

- **编译器**: GCC 4.8+ 或 Clang 3.4+
- **系统**: Linux、macOS 或 Windows（MinGW/MSYS2）
- **工具**: Make

### 编译和运行

1. **克隆或下载项目**
   ```bash
   cd mysocket
   ```

2. **编译项目**
   ```bash
   make all
   ```

3. **运行测试**
   ```bash
   make test
   ```

4. **运行示例**
   ```bash
   # 运行 TCP 服务器示例
   ./bin/server_example
   
   # 运行 TCP 客户端示例
   ./bin/client_example
   
   # 运行 UDP 通信示例
   ./bin/udp_example
   ```

### Make 命令说明

- `make all`: 编译所有源码、测试和示例
- `make libmysocket`: 只编译静态库
- `make tests`: 只编译测试程序
- `make examples`: 只编译示例程序
- `make test`: 编译并运行测试
- `make clean`: 清理编译文件

## API 使用指南

### 1. 初始化和清理

```c
#include "mysocket.h"

// 初始化 Socket 系统
int result = mysocket_init();
if (result != 0) {
    printf("初始化失败\\n");
    return -1;
}

// 使用 Socket...

// 清理 Socket 系统
mysocket_cleanup();
```

### 2. TCP 服务器示例

```c
// 创建 TCP Socket
int server_fd = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// 绑定地址
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_addr = 0;  // INADDR_ANY
addr.sin_port = mysocket_htons(8080);
mysocket_bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));

// 开始监听
mysocket_listen(server_fd, 5);

// 接受连接
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);
int client_fd = mysocket_accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

// 数据收发
char buffer[1024];
ssize_t received = mysocket_recv(client_fd, buffer, sizeof(buffer), 0);
mysocket_send(client_fd, "Hello", 5, 0);

// 关闭连接
mysocket_close(client_fd);
mysocket_close(server_fd);
```

### 3. TCP 客户端示例

```c
// 创建 TCP Socket
int client_fd = mysocket_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// 连接服务器
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr = mysocket_inet_addr("127.0.0.1");
server_addr.sin_port = mysocket_htons(8080);
mysocket_connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

// 数据收发
mysocket_send(client_fd, "Hello Server", 12, 0);
char buffer[1024];
mysocket_recv(client_fd, buffer, sizeof(buffer), 0);

// 关闭连接
mysocket_close(client_fd);
```

### 4. UDP 通信示例

```c
// 创建 UDP Socket
int udp_fd = mysocket_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

// 绑定本地地址
struct sockaddr_in local_addr;
local_addr.sin_family = AF_INET;
local_addr.sin_addr = 0;
local_addr.sin_port = mysocket_htons(9001);
mysocket_bind(udp_fd, (struct sockaddr*)&local_addr, sizeof(local_addr));

// 发送数据到指定地址
struct sockaddr_in target_addr;
target_addr.sin_family = AF_INET;
target_addr.sin_addr = mysocket_inet_addr("127.0.0.1");
target_addr.sin_port = mysocket_htons(9002);
mysocket_sendto(udp_fd, "UDP Message", 11, 0, 
                (struct sockaddr*)&target_addr, sizeof(target_addr));

// 接收数据
char buffer[1024];
struct sockaddr_in src_addr;
socklen_t src_len = sizeof(src_addr);
mysocket_recvfrom(udp_fd, buffer, sizeof(buffer), 0,
                  (struct sockaddr*)&src_addr, &src_len);

mysocket_close(udp_fd);
```

## 核心概念解析

### 1. Socket 结构体

```c
struct mysocket {
    int fd;                     // 文件描述符
    int family;                 // 协议族 (AF_INET)
    int type;                   // Socket 类型 (SOCK_STREAM/SOCK_DGRAM)
    int protocol;               // 协议 (IPPROTO_TCP/IPPROTO_UDP)
    socket_state_t state;       // Socket 状态
    tcp_state_t tcp_state;      // TCP 状态
    
    struct sockaddr_in local_addr;   // 本地地址
    struct sockaddr_in peer_addr;    // 对端地址
    
    // 缓冲区
    char *send_buffer;          // 发送缓冲区
    char *recv_buffer;          // 接收缓冲区
    // ... 其他字段
};
```

### 2. TCP 状态机

项目实现了简化的 TCP 状态机，包含以下状态：

- `TCP_CLOSED`: 关闭状态
- `TCP_LISTEN`: 监听状态
- `TCP_SYN_SENT`: 已发送 SYN
- `TCP_SYN_RECV`: 已接收 SYN
- `TCP_ESTABLISHED`: 连接建立
- `TCP_FIN_WAIT1`: 等待 FIN 确认
- `TCP_FIN_WAIT2`: 等待对端 FIN
- `TCP_CLOSE_WAIT`: 等待应用层关闭
- `TCP_LAST_ACK`: 等待最后 ACK
- `TCP_TIME_WAIT`: TIME_WAIT 状态

### 3. 缓冲区管理

- **发送缓冲区**: 应用数据先写入发送缓冲区，再统一发送
- **接收缓冲区**: 网络数据先进入接收缓冲区，应用按需读取
- **自动扩展**: 缓冲区支持动态扩展（可选功能）
- **线程安全**: 缓冲区操作使用锁保护

### 4. 错误处理

```c
// 错误码
#define MYSOCKET_OK             0
#define MYSOCKET_ERROR          -1
#define MYSOCKET_EAGAIN         -2
#define MYSOCKET_EINVAL         -3
#define MYSOCKET_EADDRINUSE     -4
#define MYSOCKET_ECONNREFUSED   -5

// 获取错误信息
const char* error_msg = mysocket_strerror(error_code);
```

## 学习要点

### 1. Socket 生命周期

```
创建 -> 绑定 -> 监听/连接 -> 数据传输 -> 关闭
  ↓       ↓         ↓           ↓        ↓
socket   bind    listen/     send/recv  close
                 connect
```

### 2. TCP 三次握手（简化实现）

```
客户端          服务器
   |              |
   |--- SYN ----> |
   |              |
   |<-- SYN+ACK --|
   |              |
   |--- ACK ----> |
   |              |
```

### 3. 内存管理模式

- **Socket 管理器**: 全局链表管理所有 Socket
- **自动清理**: Socket 关闭时自动释放相关资源
- **缓冲区**: 动态分配，支持扩展

### 4. 并发处理

- **线程锁**: 使用 pthread_mutex 保护共享数据
- **原子操作**: 文件描述符分配使用原子递增
- **状态同步**: Socket 状态变更同步更新

## 调试和诊断

### 1. 开启调试信息

编译时添加 DEBUG 宏定义：

```bash
make CFLAGS="-DDEBUG -Wall -Wextra -g -std=c99 -Iinclude"
```

### 2. 查看 Socket 信息

```c
// 打印 Socket 详细信息
mysocket_print_socket_info(sockfd);

// 获取 Socket 状态
int state = mysocket_get_socket_state(sockfd);
```

### 3. 错误诊断

```c
// 获取最后的错误码
int error_code = socket_get_error();
const char* error_msg = mysocket_strerror(error_code);
printf("错误: %s\\n", error_msg);
```

## 扩展学习

### 1. 可扩展功能

- [ ] 实现完整的 TCP 重传机制
- [ ] 添加 IPv6 支持
- [ ] 实现 Unix 域套接字
- [ ] 添加 epoll/select 事件模型
- [ ] 实现 SSL/TLS 支持

### 2. 性能优化

- [ ] 零拷贝数据传输
- [ ] 内存池管理
- [ ] 批量数据处理
- [ ] CPU 缓存优化

### 3. 协议扩展

- [ ] 实现 HTTP 协议解析
- [ ] 添加 WebSocket 支持
- [ ] 实现自定义协议栈

## 常见问题

### Q1: 编译时出现头文件找不到的错误？

**A**: 确保项目根目录下有 `include` 文件夹，并且 Makefile 中的 `-Iinclude` 参数正确。

### Q2: 运行测试时连接失败？

**A**: 这是正常现象。项目是模拟实现，不依赖真实网络，某些连接操作可能返回失败状态。

### Q3: 如何添加新的功能？

**A**: 
1. 在 `include/mysocket.h` 中添加新的函数声明
2. 在 `src/` 目录下创建实现文件
3. 更新 Makefile 包含新的源文件
4. 在 `tests/` 中添加测试用例

### Q4: 项目是否支持实际网络通信？

**A**: 当前版本主要用于学习，是模拟实现。要支持真实网络通信需要调用系统的网络接口。

## 贡献和反馈

这是一个学习项目，欢迎：

- 🐛 提交 Bug 报告
- 💡 提出改进建议
- 📝 完善文档
- 🔧 贡献代码

## 许可证

本项目采用 MIT 许可证。详情请查看 LICENSE 文件。

## 致谢

- Linux 内核网络栈的设计思想
- Stevens 的《Unix 网络编程》
- TCP/IP 详解系列

---

**学习愉快！如果这个项目对你理解 Socket 编程有帮助，请给个 ⭐ 星标支持！**