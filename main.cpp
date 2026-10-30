
#include <iostream>
#include <unistd.h>       // read, write, close
#include <fcntl.h>        // fcntl (虽然本例没直接用到，但网络编程常备)
#include <sys/epoll.h>    // epoll_create1, epoll_ctl, epoll_wait
#include <sys/socket.h>   // socket, bind, listen, accept
#include <netinet/in.h>   // sockaddr_in, htons, INADDR_ANY
#include <cstring>        // memset
using namespace std;

#define PORT 8080
#define MAX_EVENTS 1024

int main() {
// ================= 1. 创建 TCP Socket（买个手机） ====================
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
if(listen_fd == -1) { perror("listen_fd"); return 1; }

// ================= 2. 端口复用（防止重启时提示端口被占用） =============
int opt = 1;
setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// ================= 3. 绑定 IP 和 端口（给手机插上卡） =================
struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(PORT);
server_addr.sin_addr.s_addr = INADDR_ANY;

if(bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind");
    return 1;
}

// ================= 4. 开始监听（把手机设成响铃模式） =================
if(listen(listen_fd, SOMAXCONN) == -1) { perror("listen"); return 1; }
cout << "Echo Server 启动，监听端口: " << PORT << "\n";

// ================= 5. 创建 epoll 实例（请个大堂经理） =================
int epfd = epoll_create1(0);
if (epfd == -1) { perror("epoll_create1"); return 1; }

// ================= 6. 将 listen_fd 注册到 epoll 中 =================
struct epoll_event ev;
struct epoll_event events[MAX_EVENTS];

ev.data.fd = listen_fd;
ev.events = EPOLLIN;
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

// ================= 7. 核心事件循环（经理开始盯着了） =================
while(true) {
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if(nfds == -1) { perror("epoll_wait"); break; }

    for(int i = 0; i < nfds; ++i) {
        int current_fd = events[i].data.fd;

        if(current_fd == listen_fd) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
            if(client_fd == -1) { perror("accept"); continue; }

            cout << "[Server] 新客户端接入, fd = " << client_fd << "\n";

            ev.data.fd = client_fd;
            ev.events = EPOLLIN;
            epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
        }
        // ---------- 情况 B：某个客户端发数据来了 ----------
        else {
            char buf[1024] = {0};
            ssize_t n = read(current_fd, buf, sizeof(buf) - 1);

            if(n > 0) {
                cout << "[Server] 收到 fd " << current_fd << " 数据: " << buf;
                write(current_fd, buf, n);
            }
            else if(n == 0) {
                cout << "[Server] 客户端断开, fd = " << current_fd << "\n";
                epoll_ctl(epfd, EPOLL_CTL_DEL, current_fd, nullptr);
                close(current_fd);
            }
            else {
                perror("read");
                epoll_ctl(epfd, EPOLL_CTL_DEL, current_fd, nullptr);
                close(current_fd);
            }
        }
    }
}

    close(epfd);
    close(listen_fd);

    return 0;
}