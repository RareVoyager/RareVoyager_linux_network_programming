#include "wrap.h"

static int create_listen_socket(uint16_t port)
{
    int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        std::perror("socket");
        return -1;
    }

    int on = 1;
    if (::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        std::perror("setsockopt(SO_REUSEADDR)");
        ::close(listenfd);
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(listenfd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::perror("bind");
        ::close(listenfd);
        return -1;
    }

    if (::listen(listenfd, 128) < 0)
    {
        std::perror("listen");
        ::close(listenfd);
        return -1;
    }

    return listenfd;
}

int main(int argc, char **argv)
{
    uint16_t port = 9090;
    if (argc >= 2)
    {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    int listenfd = create_listen_socket(port);
    if (listenfd < 0)
        return 1;

    std::cout << "[server] listening on 0.0.0.0:" << port << "\n";
    std::cout << "[server] try:  nc 127.0.0.1 " << port << "\n";

    FdArray clients;

    while (true)
    {
        fd_set readfds;

        // 1) 先把监听 socket 加到 readfds
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);

        int maxfd = listenfd;

        // 2) 把客户端集合也加入 readfds（通过 FdArray 的 fdset_ 生成）
        clients.setFdSet();
        fd_set cset = clients.getFdSet();
        for (int fd = 0; fd <= clients.maxFd(); ++fd)
        {
            if (FD_ISSET(fd, &cset))
            {
                FD_SET(fd, &readfds);
                if (fd > maxfd)
                    maxfd = fd;
            }
        }

        int nready = ::select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (nready < 0)
        {
            if (errno == EINTR)
                continue;
            std::perror("select");
            break;
        }

        // 3) 新连接
        if (FD_ISSET(listenfd, &readfds))
        {
            sockaddr_in peer{};
            socklen_t len = sizeof(peer);
            int connfd = ::accept(listenfd, (sockaddr *)&peer, &len);
            if (connfd >= 0)
            {
                char ip[INET_ADDRSTRLEN]{};
                ::inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
                std::cout << "[server] accept fd=" << connfd
                          << " from " << ip << ":" << ntohs(peer.sin_port) << "\n";

                clients.addFd(connfd);
            }
            else
            {
                std::perror("accept");
            }
        }

        // 4) 处理客户端可读（用你的 for_each_ready）
        clients.for_each_ready(readfds, [&](int fd)
                               {
            char buf[1024];
            ssize_t n = ::recv(fd, buf, sizeof(buf) - 1, 0);
            if (n == 0)
            {
                std::cout << "[server] peer closed fd=" << fd << "\n";
                clients.removeFd(fd);
                ::close(fd);
                return;
            }
            if (n < 0)
            {
                std::cout << "[server] recv error fd=" << fd << " errno=" << errno
                          << " (" << std::strerror(errno) << ")\n";
                clients.removeFd(fd);
                ::close(fd);
                return;
            }

            buf[n] = '\0';
            std::cout << "[server] fd=" << fd << " recv: " << buf;

            std::string resp = "echo: " + std::string(buf, buf + n);
            ::send(fd, resp.data(), resp.size(), 0); });
    }

    ::close(listenfd);
    return 0;
}
