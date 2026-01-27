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
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sevr_addr;
    bzero(&sevr_addr, sizeof(sevr_addr));
    sevr_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sevr_addr.sin_addr);
    sevr_addr.sin_port = htons(SEVR_PORT);

    char buf[BUFSIZ];
    int n;
    while (fgets(buf, BUFSIZ, stdin) != NULL)
    {
        n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&sevr_addr, sizeof(sevr_addr));
        if (n == -1)
        {
            Wrap::Die("sendto error");
        }
        n = recvfrom(sockfd, buf, BUFSIZ, 0, NULL, 0);
        if (n == -1)
        {
            Wrap::Die("recvform error");
        }
        write(STDOUT_FILENO, buf, n);
    }

    close(sockfd);
    return 0;
}