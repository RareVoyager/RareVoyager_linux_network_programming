#include "wrap.h"

/**
 * @brief:          阻塞等待客户端建立连接，成功返回一个与客户端连接成功的socket套接字
 * @param fd:       监听的socket
 * @param sa:       输出参数，客户端地址
 * @param salenptr: 输入/输出参数: 地址结构大小
 */
int Wrap::Accept(int fd, sockaddr *sa, socklen_t *salenptr)
{
    int ret = ::accept(fd, sa, salenptr);
    if (ret == -1)
    {
        Wrap::Die("accept");
    }
    return ret;
}

/**
 * @brief:          给socket 绑定一个地址结构(Ip + port)
 * @param fd:       socket() 返回的 fd
 * @param sa:       地址结构（IPv4 用 sockaddr_in）
 * @param salen:    地址结构大小（sizeof(struct sockaddr_in)）
 */
int Wrap::Bind(int fd, const sockaddr *sa, socklen_t salen)
{
    int ret = bind(fd, sa, salen);
    if (ret == -1)
    {
        Wrap::Die("bind");
    }
    return ret;
}
/**
 * @brief:			    与服务器建立连接，使用现有的socket
 * @param fd			客户端 socket
 * @param sa			服务器地址
 * @param addrlen		地址结构大小
 */
int Wrap::Connect(int fd, const sockaddr *sa, socklen_t salen)
{
    int ret = connect(fd, sa, salen);
    if (ret == -1)
    {
        Wrap::Die("connect");
    }
    return ret;
}

/**
 * @brief: 			    设置同时与服务器连接的上限数量(可以同时进行三次握手的客户端数量)
 * @param sockfd	    已 bind 的 socket
 * @param backlog  	    半连接队列最大长度（并发连接上限）系统自动设置为128
 */
int Wrap::Listen(int fd, int backlog)
{
    int ret = listen(fd, backlog);
    if (ret == -1)
    {
        Wrap::Die("listen");
    }
    return ret;
}

/**
 * @brief:			    创建一个套接字
 * @param domain:		协议族，如 AF_INET（IPv4）、AF_INET6
 * @param type:			套接字类型，如 SOCK_STREAM（TCP）、SOCK_DGRAM（UDP）
 * @param protocol:		协议号，通常写 0（让系统自动选择）
 */
int Wrap::Socket(int family, int type, int protocol)
{
    int fd = socket(family, type, protocol);
    if (fd == -1)
    {
        Wrap::Die("socket");
    }
    return fd;
}

/**
 * @brief       关闭一个套接字连接
 * @param fd    套接字
 */
int Wrap::Close(int fd)
{
    int ret = close(fd);
    if (ret == -1)
    {
        Wrap::Die("close");
    }
    return ret;
}

ssize_t Wrap::Read(int fd, const void *ptr, size_t nbytes)
{
    void *buf = const_cast<void *>(ptr);

    while (true)
    {
        ssize_t n = ::read(fd, buf, nbytes);
        if (n >= 0)
            return n;
        if (errno == EINTR)
            continue; // 被信号打断，重试
        Die("read");
    }
}

ssize_t Wrap::Write(int fd, const void *ptr, size_t nbytes)
{
    while (true)
    {
        ssize_t n = ::write(fd, ptr, nbytes);
        if (n >= 0)
            return n;
        if (errno == EINTR)
            continue;
        Die("write");
    }
}

// -------------------- Readn / Writen：保证读/写满 n 字节（除非 EOF/错误）--------------------

ssize_t Wrap::Readn(int fd, void *vptr, size_t n)
{
    size_t nleft = n;
    char *ptr = static_cast<char *>(vptr);

    while (nleft > 0)
    {
        ssize_t nread = ::read(fd, ptr, nleft);
        if (nread > 0)
        {
            nleft -= static_cast<size_t>(nread);
            ptr += nread;
        }
        else if (nread == 0)
        {
            // EOF：提前结束
            break;
        }
        else
        { // nread < 0
            if (errno == EINTR)
                continue;
            Die("readn/read");
        }
    }
    return static_cast<ssize_t>(n - nleft); // 实际读到的字节数
}

ssize_t Wrap::Writen(int fd, const void *vptr, size_t n)
{
    size_t nleft = n;
    const char *ptr = static_cast<const char *>(vptr);

    while (nleft > 0)
    {
        ssize_t nwritten = ::write(fd, ptr, nleft);
        if (nwritten > 0)
        {
            nleft -= static_cast<size_t>(nwritten);
            ptr += nwritten;
        }
        else if (nwritten == 0)
        {
            // write 返回 0 很少见，通常视为无法继续写
            Die("writen/write_return_0");
        }
        else
        { // nwritten < 0
            if (errno == EINTR)
                continue;
            Die("writen/write");
        }
    }
    return static_cast<ssize_t>(n);
}

// -------------------- ReadLine：按行读取（最多 maxlen-1 字符，保证以 '\0' 结尾）--------------------
// 经典做法：用 my_read 做一个“带内部缓冲”的单字节读取，减少系统调用次数。

ssize_t Wrap::my_read(int fd, char *ptr)
{
    // 注意：这是“带静态内部缓冲”的实现，不是线程安全的（单线程网络练习完全 OK）。
    static char read_buf[4096];
    static char *read_ptr = nullptr;
    static int read_cnt = 0;

    while (read_cnt <= 0)
    {
        ssize_t n = ::read(fd, read_buf, sizeof(read_buf));
        if (n > 0)
        {
            read_cnt = static_cast<int>(n);
            read_ptr = read_buf;
            break;
        }
        if (n == 0)
            return 0; // EOF
        if (errno == EINTR)
            continue; // 被信号打断，重试
        Die("my_read/read");
    }

    // 从内部缓冲拿 1 个字节给调用者
    *ptr = *read_ptr++;
    --read_cnt;
    return 1;
}

ssize_t Wrap::ReadLine(int fd, void *vptr, size_t maxlen)
{
    if (maxlen == 0)
        return 0;

    char *ptr = static_cast<char *>(vptr);
    size_t n;

    for (n = 1; n < maxlen; ++n)
    {
        char c;
        ssize_t rc = my_read(fd, &c);
        if (rc == 1)
        {
            *ptr++ = c;
            if (c == '\n')
                break; // 读到换行结束
        }
        else if (rc == 0)
        {
            // EOF：如果已经读了一些字节则返回已读长度，否则返回 0
            if (n == 1)
                return 0;
            break;
        }
        else
        {
            // 正常情况下 my_read 不会返回 -1（它失败会 Die），这里只是保险
            Die("readline/my_read");
        }
    }

    *ptr = '\0';
    return static_cast<ssize_t>(n);
}

void FdArray::addFd(int fd)
{
    fds_.push_back(fd);
    if (fd > maxfd_)
    {
        maxfd_ = fd;
    }
}

void FdArray::removeFd(int fd)
{
    for (auto it = fds_.begin(); it != fds_.end(); ++it)
    {
        if (*it == fd)
        {
            fds_.erase(it);
            break;
        }
    }
    // 重新计算 maxfd（fd 数量小，代价可接受）
    if (fd == maxfd_)
    {
        maxfd_ = -1;
        for (int f : fds_)
        {
            if (f > maxfd_)
                maxfd_ = f;
        }
    }
}

int FdArray::maxFd() const
{
    return maxfd_;
}

void FdArray::setFdSet()
{
    FD_ZERO(&fdset_);
    for (int fd : fds_)
    {
        FD_SET(fd, &fdset_);
    }
}

fd_set FdArray::getFdSet() const
{
    return fdset_;
}
