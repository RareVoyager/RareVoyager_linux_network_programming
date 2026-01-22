#ifndef __WRAP_H_
#define __WRAP_H_

#include <iostream>
#include <sys/socket.h> // socket, bind, sockaddr
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton
#include <unistd.h>     // close
#include <cstring>
#include <bits/socket.h>
#include <assert.h>
class Wrap
{

private:
    Wrap() = delete;
    ~Wrap() = delete;

public:
    static void Die(const char *api)
    {
        std::cerr << api << " failed, errno=" << errno
                  << " (" << std::strerror(errno) << ")\n";
        std::exit(EXIT_FAILURE);
    }

    static int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
    static int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
    static int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
    static int Listen(int fd, int backlog);
    static int Socket(int family, int type, int protocol);
    static int Close(int fd);

    static ssize_t Read(int fd, const void *ptr, size_t nbytes);
    static ssize_t Write(int fd, const void *ptr, size_t nbytes);
    static ssize_t Readn(int fd, void *vptr, size_t n);
    static ssize_t Writen(int fd, const void *vptr, size_t n);
    static ssize_t ReadLine(int fd, void *ptr, size_t maxlen);
    static ssize_t my_read(int fd, char *ptr);
};

#endif