#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "wrap.h"
#include <vector>

#define SEVR_PORT 9527

int main()
{
    // socket ,bind,listen
    int lfd, connectfd;
    char buf[BUFSIZ];
    lfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sevr_addr{};
    bzero(&sevr_addr, sizeof(sevr_addr));
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);
    sevr_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Wrap::Bind(lfd, (struct sockaddr *)&sevr_addr, sizeof(sevr_addr));

    Wrap::Listen(lfd, 128);

    fd_set readfds, allsets;
    FD_ZERO(&allsets);
    FD_SET(lfd, &allsets);
    int ret, maxfd;
    maxfd = lfd;

    struct sockaddr_in clit_addr{};
    socklen_t clit_addr_len;
    while (true)
    {
        readfds = allsets;
        ret = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (ret < 0)
        {
            Wrap::Die("select error");
        }

        if (FD_ISSET(lfd, &readfds))
        {
            clit_addr_len = sizeof(clit_addr);
            connectfd = Wrap::Accept(lfd, (struct sockaddr *)&clit_addr, &clit_addr_len);
            FD_SET(connectfd, &allsets);

            if (maxfd < connectfd)
            {
                maxfd = connectfd;
            }
            if (--ret == 0)
            {
                continue;
            }
        }

        // for (int i = lfd + 1; i <= maxfd; i++)
        // {
        //     if (FD_ISSET(i, &readfds))
        //     {
        //         int n = Wrap::Read(i, buf, sizeof(buf));
        //         if (n == 0)
        //         {
        //             Wrap::Close(i);
        //             FD_CLR(i, &allsets);
        //         }
        //         else if (n == -1)
        //         {
        //             Wrap::Die("read error");
        //         }

        //         for (int j = 0; j < n; j++)
        //         {
        //             buf[j] = toupper(buf[j]);
        //         }
        //         Wrap::Write(i, buf, n);
        //         Wrap::Write(STDOUT_FILENO, buf, n);
        //     }
        // }

        for (int i = lfd + 1; i <= maxfd; ++i)
        {
            if (!FD_ISSET(i, &readfds))
                continue;

            int n = ::read(i, buf, sizeof(buf));
            if (n == 0)
            {
                // 对端关闭
                Wrap::Close(i);
                FD_CLR(i, &allsets);
            }
            else if (n < 0)
            {
                // 对端重置：当作掉线处理
                if (errno == ECONNRESET)
                {
                    Wrap::Close(i);
                    FD_CLR(i, &allsets);
                }
                else if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    // 关掉这个连接
                    perror("read");
                    Wrap::Close(i);
                    FD_CLR(i, &allsets);
                }
            }
            else
            {
                for (int j = 0; j < n; ++j)
                {
                    buf[j] = static_cast<char>(toupper(static_cast<unsigned char>(buf[j])));
                }
                Wrap::Write(i, buf, n);
                Wrap::Write(STDOUT_FILENO, buf, n);
            }

            if (--ret == 0)
                break; // 本轮 ready 的 fd 都处理完了
        }
    }
    Wrap::Close(lfd);
    return 0;
}