#include <iostream>
#include <sys/socket.h> // socket, bind, sockaddr
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton
#include <unistd.h>     // close
#include <signal.h>
#include <sys/wait.h>
#include "wrap.h"
#include <sys/epoll.h>

#define SEVR_PORT 9527
#define EPOLL_MAX_SIZE 1024

/**
 * @brief: epoll_service 服务端代码
 * @TODO: 未来用C++ 面向对象的思想来重写代码 目前写法是C 的面向过程写法
 */
int main()
{
    // 1.创建监听套接字
    int lfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);

    // 设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定 + 监听
    struct sockaddr_in sevr_addr{};
    bzero(&sevr_addr, sizeof(sevr_addr));
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);
    sevr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t len = sizeof(sevr_addr);
    Wrap::Bind(lfd, (struct sockaddr *)&sevr_addr, len);
    Wrap::Listen(lfd, 128);

    // 2.创建根节点
    int epfd = epoll_create(EPOLL_MAX_SIZE);
    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = lfd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &event);
    if (ret == -1)
    {
        Wrap::Die("epoll_ctl add listen fd error");
    }

    // 3.事件循环
    // 返回的事件数量
    struct epoll_event events[EPOLL_MAX_SIZE];
    int n = 0;
    while (true)
    {
        // 设置非阻塞模式
        n = epoll_wait(epfd, events, EPOLL_MAX_SIZE, -1);
        // 错误了, 直接退出
        if (n == -1)
        {
            Wrap::Die("epoll_wait error");
        }

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;
            if (fd == lfd)
            {
                // 处理accept cfd 连接 添加到events里面
                struct sockaddr_in clit_addr{};
                socklen_t clit_len = sizeof(clit_addr);
                // TODO: 未来可能要调整Accept 为while 循环添加
                int cfd = Wrap::Accept(lfd, (struct sockaddr *)&clit_addr, &clit_len);

                // 打印客户端信息
                char ip[INET_ADDRSTRLEN]{0};
                inet_ntop(AF_INET, &clit_addr.sin_addr, ip, sizeof(ip));
                uint16_t port = ntohs(clit_addr.sin_port);
                std::cout << "[NEW] client connected: "
                          << ip << ":" << port
                          << " (cfd=" << cfd << ")\n";

                struct epoll_event clit_event{};
                clit_event.events = EPOLLIN;
                clit_event.data.fd = cfd;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &clit_event);
                if (ret == -1)
                {
                    Wrap::Die("epoll_ctl add conncet fd error");
                }
            }
            else
            {
                //* 处理EPOLLIN / EPOLLOUT
                // EPOLLIN 等是个位掩码，类似于 0x01 0x02这样。仅用 == 容易漏判
                if (events[i].events & EPOLLIN)
                {
                    // 处理读
                    char buf[BUFSIZ];
                    ssize_t nread = Wrap::Read(fd, buf, sizeof(buf));
                    std::cout << "[RECV fd=" << fd << "] ";
                    std::cout.write(buf, nread);
                    std::cout << std::flush;
                    if (nread > 0)
                    {
                        for (int c = 0; c < nread; c++)
                        {
                            buf[c] = ::toupper(buf[c]);
                        }
                        // 回写给客户端
                        Wrap::Write(fd, buf, nread);
                    }
                    else if (nread == 0)
                    {
                        // 对端关闭，从events中删除对应的fd
                        std::cout << "对端关闭" << std::endl;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        Wrap::Close(fd);
                    }
                    else
                    {
                        // 处理error
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                        Wrap::Close(fd);
                    }
                }
                // if (events[i].events & EPOLLOUT)
                // {
                //     // 处理之前没写完,
                //     char buf[BUFSIZ];
                //     ssize_t nwrite = Wrap::Write(fd, buf, sizeof(buf));
                //     if (nwrite > 0)
                //     {
                //         send_pos += n;
                //         if (send_pos == buf.size())
                //         {
                //             // 全部写完
                //             outbuf.clear();
                //             send_pos = 0;

                //             // 关闭 EPOLLOUT
                //             struct epoll_event ev{};
                //             ev.events = EPOLLIN;
                //             ev.data.fd = fd;
                //             epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
                //         }
                //     }
                //     else if (nwrite == -1 && errno == EAGAIN)
                //     {
                //         // 写缓冲区满，等下一次 EPOLLOUT
                //         return;
                //     }
                //     else
                //     {
                //         epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                //         Wrap::Close(fd);
                //     }
                // }
            }
        }
    }
    Wrap::Close(lfd);
    return 0;
}