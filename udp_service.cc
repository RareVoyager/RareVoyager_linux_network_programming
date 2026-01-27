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
int main()
{
    int lfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sevr_addr, clit_addr;
    bzero(&sevr_addr, sizeof(sevr_addr));
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);
    sevr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    std::printf("Accepting connection……");
    bind(lfd, (struct sockaddr *)&sevr_addr, sizeof(sevr_addr));

    socklen_t clit_len;
    int n = 0;
    char buf[BUFSIZ];
    char str[INET_ADDRSTRLEN];
    while (true)
    {
        clit_len = sizeof(clit_len);
        n = recvfrom(lfd, buf, BUFSIZ, 0, (struct sockaddr *)&clit_addr, &clit_len);
        if (n == -1)
        {
            Wrap::Die("service recvform error");
        }
        std::printf("received from %s at PoRT %d\n",
                    inet_ntop(AF_INET, &clit_addr.sin_addr, str, sizeof(str)),
                    ntohs(clit_addr.sin_port));

        for (int i = 0; i < n; i++)
        {
            buf[i] = ::toupper(buf[i]);
        }
        n = sendto(lfd, buf, n, 0, (struct sockaddr *)&clit_addr, sizeof(clit_addr));
        if (n == -1)
        {
            Wrap::Die("service sendto error");
        }
    }
    close(lfd);

    return 0;
}