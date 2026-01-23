#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "wrap.h"


#define SEVR_PORT 9527
int main()
{
    // 创建客户端套接字
    int cfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);
    int ret = 0;
    struct sockaddr_in clit_addr{};
    clit_addr.sin_family = AF_INET;
    clit_addr.sin_port = htons(SEVR_PORT);
    std::cout << htons(SEVR_PORT) << std::endl;

    ret = inet_pton(AF_INET, "127.0.0.1", &clit_addr.sin_addr.s_addr);
    if (ret != 1)
    {
        std::cerr << "create s_addr error" << std::endl;
        close(cfd);
        return 0;
    }

    int lfd = Wrap::Connect(cfd, (struct sockaddr *)&clit_addr, sizeof(clit_addr));

    int count = 12;
    char buf[BUFSIZ];
    while (--count)
    {
        ssize_t w = Wrap::Write(cfd, "hello world\n", 13);
        if (w <= 0)
        {
            perror("error");
            break;
        }
        ssize_t r = Wrap::Read(cfd, buf, sizeof(buf));
        if (r <= 0)
        {
            perror("error to read");
            break;
        }
        Wrap::Write(STDOUT_FILENO, buf, (size_t)r);
        sleep(1);
    }
    Wrap::Close(cfd);
    std::cout << "task finished!!" << std::endl;
    return 0;
}