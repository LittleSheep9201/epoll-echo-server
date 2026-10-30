# Epoll TCP Echo Server

基于 C++ 和 Linux epoll 实现的单线程高并发回显服务器。

## 核心技术
- **C++ / Linux**：基于 POSIX API 编写，运行于 WSL 2 (Ubuntu) 环境。
- **epoll**：采用事件驱动模型（LT 模式），单线程管理并发连接。
- **Socket API**：熟练使用 socket、bind、listen、accept 建立 TCP 连接。

## 核心功能
- 支持多客户端同时连接，单线程无阻塞处理并发请求。
- 完整的资源生命周期管理：客户端断开自动触发 read 返回 0，执行 `epoll_ctl(DEL)` 并 `close(fd)`，无文件描述符泄漏。

## 快速开始
```bash
g++ main.cpp -o server
./server