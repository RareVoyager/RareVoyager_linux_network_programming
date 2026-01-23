#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <cctype>  // toupper
#include <cstring> // memset
#include "wrap.h"
#include <memory>

#define SEVR_PORT 9527

// 线程参数
struct ThreadArg
{
    int cfd;
    sockaddr_in client_addr;
};

// 线程入口函数：处理一个客户端连接
static void *client_thread(void *arg)
{
    // 接管参数
    ThreadArg *p = static_cast<ThreadArg *>(arg);
    int cfd = p->cfd;

    // 打印客户端地址
    char ip[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &p->client_addr.sin_addr, ip, sizeof(ip));
    std::cout << "client connected: " << ip << ":" << ntohs(p->client_addr.sin_port) << std::endl;

    char buf[1024];

    while (true)
    {
        int ret = Wrap::Read(cfd, buf, sizeof(buf));
        if (ret == 0)
        {
            // 客户端关闭
            Wrap::Close(cfd);
            return nullptr;
        }
        else if (ret < 0)
        {
            // 读失败
            Wrap::Close(cfd);
            return nullptr;
        }

        for (int i = 0; i < ret; i++)
        {
            buf[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(buf[i])));
        }

        Wrap::Write(cfd, buf, ret);
        Wrap::Write(STDOUT_FILENO, buf, ret);
    }
}

int main()
{
    // （推荐）避免客户端异常断开时，服务器因 SIGPIPE 直接退出
    // Linux 下对已关闭的 socket write 可能触发 SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    int lfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in sevr_addr{};
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);
    sevr_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // （可选但推荐）端口复用，避免 TIME_WAIT 导致 bind 失败
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Wrap::Bind(lfd, (struct sockaddr *)&sevr_addr, sizeof(sevr_addr));
    Wrap::Listen(lfd, 128);

    while (true)
    {
        sockaddr_in clit_addr{};
        socklen_t clit_addr_len = sizeof(clit_addr);

        int cfd = Wrap::Accept(lfd, (struct sockaddr *)&clit_addr, &clit_addr_len);
        std::cout << "接受到参数" << cfd << std::endl;
        // 给线程准备参数（堆上分配，线程里 delete）
        ThreadArg *arg = new ThreadArg;
        arg->cfd = cfd;
        arg->client_addr = clit_addr;

        pthread_t tid;
        /**
         * @param tid           线程ID
         * @param attr          线程属性（nullptr表示默认）
         * @param start_routine 线程入口函数
         * @param arg           传递给线程的参数
         * @return 0成功，1失败
         */
        int r = pthread_create(&tid, nullptr, client_thread, arg);
        if (r != 0)
        {
            // 创建线程失败：关闭连接，释放参数
            Wrap::Close(cfd);
            delete arg;
            std::cerr << "pthread_create failed: " << r << std::endl;
            continue;
        }

        // 分离线程：无需 pthread_join，线程结束自动回收资源
        pthread_detach(tid);
    }

    Wrap::Close(lfd);
    return 0;
}
