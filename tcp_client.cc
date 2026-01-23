#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "wrap.h"

#define SEVR_PORT 9527

int main()
{
    int cfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in sevr_addr{};
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);

    int ret = inet_pton(AF_INET, "127.0.0.1", &sevr_addr.sin_addr);
    if (ret != 1)
    {
        std::cerr << "inet_pton failed\n";
        Wrap::Close(cfd);
        return 1;
    }

    // Connect 成功返回 0，失败 Wrap::Die 直接退出
    Wrap::Connect(cfd, (sockaddr *)&sevr_addr, sizeof(sevr_addr));

    int count = 12;
    char buf[BUFSIZ];

    while (--count)
    {
        // 你写 13 也可以，但建议 strlen 更不容易写错
        const char *msg = "hello world\n";
        ssize_t w = Wrap::Write(cfd, msg, std::strlen(msg));
        if (w < 0)
        {
            perror("write");
            break;
        }

        // 读回显
        ssize_t r = Wrap::Read(cfd, buf, sizeof(buf));
        if (r > 0)
        {
            Wrap::Write(STDOUT_FILENO, buf, (size_t)r);
        }
        else if (r == 0)
        {
            // 对端正常关闭（EOF），不是错误
            std::cout << "server closed connection (EOF)\n";
            break;
        }
        else
        {
            // r < 0 才是错误
            perror("read");
            break;
        }

        sleep(1);
    }

    Wrap::Close(cfd);
    std::cout << "task finished!!\n";
    return 0;
}
