#include <iostream>
#include <sys/socket.h> // socket, bind, sockaddr
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton
#include <unistd.h>     // close
#include <signal.h>
#include <sys/wait.h>
#include "wrap.h"

#define SEVR_PORT 9527
bool isError(int ret, const std::string &msg)
{
    if (ret == -1)
    {
        std::cerr << msg << " error" << std::endl;
        return false;
    }
    std::cout << "create " << msg << " success" << std::endl;
    return true;
}

void catch_child()
{
    while (waitpid(0, nullptr, WNOHANG) > 0)
        ;
}

int main()
{
    char buf[1024];

    pid_t pid;
    int lfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sevr_addr{};
    struct sockaddr_in clit_addr{};
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);
    sevr_addr.sin_addr.s_addr = htons(INADDR_ANY);

    Wrap::Bind(lfd, (struct sockaddr *)&sevr_addr, sizeof(sevr_addr));

    Wrap::Listen(lfd, 128);

    socklen_t clit_addr_len = sizeof(clit_addr);
    int cfd;
    // 多进程实现:
    while (true)
    {
        cfd = Wrap::Accept(lfd, (struct sockaddr *)&clit_addr, &clit_addr_len);
        pid = fork();
        if (pid < 0)
        {
            Wrap::Die("create pid failed");
        }
        else if (pid == 0)
        {
            Wrap::Close(lfd);
            break;
        }
        else
        {
            struct sigaction act;
            act.sa_restorer = catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            int r = sigaction(SIGCHLD, &act, NULL);
            if (r == -1)
            {
                Wrap::Die("sigaction error");
            }
            Wrap::Close(cfd);
            continue;
        }
    }

    int ret;
    if (pid == 0)
    {
        while (true)
        {
            ret = Wrap::Read(cfd, buf, sizeof(buf));
            if (ret == 0)
            {
                close(0);
                exit(1);
            }
            for (int i = 0; i < ret; i++)
            {
                buf[i] = toupper(buf[i]);
            }
            Wrap::Write(cfd, buf, ret);
            Wrap::Write(STDOUT_FILENO, buf, ret);
        }
    }

    // TODO: 添加回收逻辑
    Wrap::Close(lfd);
    Wrap::Close(cfd);
    return 0;
}