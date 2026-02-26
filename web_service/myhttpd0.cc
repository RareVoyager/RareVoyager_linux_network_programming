#include <iostream>
#include <sys/socket.h> // socket, bind, sockaddr
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton
#include <unistd.h>     // close
#include <signal.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include "error.h"
#define MAXSIZE 2048

// 发送 HTTP 错误响应
void send_error(int cfd,
                int status,
                const char *title,
                const char *text)
{
    char buf[4096];

    // 生成 HTML body
    int body_len = snprintf(buf, sizeof(buf),
                            "<html><head><title>%d %s</title></head>\n"
                            "<body bgcolor=\"#cc99cc\">\n"
                            "<h4 align=\"center\">%d %s</h4>\n"
                            "%s\n"
                            "<hr>\n"
                            "</body></html>\n",
                            status, title,
                            status, title,
                            text);

    if (body_len < 0)
        return;

    // 生成 header
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: text/html; charset=utf-8\r\n"
                              "Content-Length: %d\r\n"
                              "Connection: close\r\n"
                              "\r\n",
                              status, title, body_len);

    if (header_len < 0)
        return;

    // 发送 header
    send(cfd, header, header_len, 0);

    // 发送 body
    send(cfd, buf, body_len, 0);
}

int get_line(int cfd, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    // 一次读一个字节，直到 '\n' 或缓冲区满
    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(cfd, &c, 1, 0);

        if (n > 0)
        {
            if (c == '\r')
            {
                // 偷看下一个字符（不取走）
                n = recv(cfd, &c, 1, MSG_PEEK);

                if ((n > 0) && (c == '\n'))
                {
                    // 真正取走 '\n'
                    recv(cfd, &c, 1, 0);
                }
                else
                {
                    // 只有 '\r'，当成 '\n'
                    c = '\n';
                }
            }

            buf[i++] = c;
        }
        else
        {
            // 客户端关闭或出错，强制结束
            c = '\n';
        }
    }
    // c++ 字符串结束标志位
    buf[i] = '\0';

    return -1 == n ? -1 : i;
}

int init_listen_fd(int port, int epfd)
{
    // 创建监听套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error");
        exit(1);
    }
    // 创建服务器地址结构IP + port
    struct sockaddr_in srv_addr;
    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // 端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // 给lfd绑定地址结构
    int ret = bind(lfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (ret == -1)
    {
        perror("bind error");
        exit(1);
    }
    // 设置监听上限
    ret = listen(lfd, 128);

    // lfd 添加到epoll 树

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

    if (ret == -1)
    {
        perror("epoll_ctl add lfd error");
        exit(1);
    }
    return lfd;
}

void do_accept(int lfd, int epfd)
{

    struct sockaddr_in clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int cfd = accept(lfd, (struct sockaddr *)&clt_addr, &clt_addr_len);
    if (cfd == -1)
    {
        perror("accept error");
        exit(1);
    }

    // 打印客户端IP+port
    char client_ip[64] = {0};
    printf("New Client IP: %s, Port: %d, cfd = %d\n",
           inet_ntop(AF_INET, &clt_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)), ntohs(clt_addr.sin_port), cfd);
    // 设置 cfd 非阻塞
    // fcntl 文件描述符进行各种控制操作
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    // 将新节点cfd挂到epoll监听树上
    struct epoll_event ev;
    ev.data.fd = cfd;

    ev.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl add cfd error");
        exit(1);
    }
}

void disconnect(int cfd, int epfd)
{
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
    if (ret != 0)
    {
        std::cout << "epoll_ctl error" << std::endl;
        exit(1);
    }
    close(cfd);
}

void send_respond(int cfd, int no, const char *disp, const char *type, int len)
{
    char buf[1024];

    int n = snprintf(buf, sizeof(buf),
                     "HTTP/1.1 %d %s\r\n"
                     "%s\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     no, disp, type, len);

    send(cfd, buf, n, 0);
}

void send_file(int cfd, const char *file)
{
    int fd = open(file, O_RDONLY);
    if (fd == -1)
    {
        send_error(cfd, 404, "Not Find", "No Such file or direntry");
        exit(1);
    }
    int n, ret;
    char buf[1024] = {0};
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        // 发送浏览器文件给
        ret = send(cfd, buf, n, 0);
        if (ret == -1)
        {
            std::cout << "error = " << errno;
            if (errno == EAGAIN)
            {
                std::cout << "----------EAGAIN" << std::endl;
                continue;
            }
            else if (errno == EINTR)
            {
                std::cout << "----------EINTR" << std::endl;
                continue;
            }
            else
            {
                perror("send error");
                exit(1);
            }
        }
    }

    close(fd);
}

void http_request(int cfd, const char *file)
{
    // 查看文件是否存在
    struct stat sbuf;
    int ret = stat(file, &sbuf);
    if (ret != 0)
    {
        // 404页面
        perror("stat");
        exit(1);
    }

    if (S_ISREG(sbuf.st_mode))
    {
        // 是一个普通文件
        send_respond(cfd, 200, "OK", " Content-Type: text/plain; charset=iso-8859-1", sbuf.st_size);
        // http 协议应答
        send_file(cfd, file);
    }
}

static void discard_headers(int cfd)
{
    char buf[1024];
    while (1)
    {
        int n = get_line(cfd, buf, sizeof(buf));
        if (n <= 0)
            break;
        if (strcmp(buf, "\n") == 0 || strcmp(buf, "\r\n") == 0)
            break; // 空行结束
    }
}

void do_read(int cfd, int epfd)
{
    char line[1024];
    int n = get_line(cfd, line, sizeof(line));

    if (n == 0)
    {
        disconnect(cfd, epfd);
        return;
    }
    if (n < 0)
    {
        return;
    } // 非阻塞下 EAGAIN 时先返回

    char method[16], path[256], protocol[16];
    sscanf(line, "%15[^ ] %255[^ ] %15[^ ]", method, path, protocol);

    discard_headers(cfd);

    if (strncasecmp(method, "GET", 3) == 0)
    {
        const char *file = (path[0] == '/') ? path + 1 : path;
        if (*file == '\0')
            file = "index.html"; // 可选：默认页
        http_request(cfd, file);
        disconnect(cfd, epfd); // 简单起见：响应完就关
    }
}

// create ctl
void epoll_run(int port)
{
    struct epoll_event all_events[MAXSIZE];
    // 创建一个epoll监听树根
    int epfd = epoll_create(MAXSIZE);
    if (epfd == -1)
    {
        perror("epoll_create error");
        exit(1);
    }
    // 创建1fd，并添加至监听树
    int lfd = init_listen_fd(port, epfd);
    while (1)
    {
        // 监听节点对应事件
        int ret = epoll_wait(epfd, all_events, MAXSIZE, -1);
        if (ret == -1)
        {
            perror("epoll_wait error");
            exit(1);
        }

        for (int i = 0; i < ret; ++i)
        {
            // 只处理读事件，其他事件默认不处理
            struct epoll_event *pev = &all_events[i]; // 不是读事件
            if (!(pev->events & EPOLLIN))
            {
                continue;
            }
            if (pev->data.fd == lfd)
            {
                // 接受连接请求
                do_accept(lfd, epfd);
            }
            else
            {
                // 读数据
                do_read(pev->data.fd, epfd);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "./a.out port path" << std::endl;
    }

    // 获取端口号 atoi()将字符串转换为int
    // atoi() 不安全 原因是 无法区分 "abc" 和 "0" 的转换
    int port = atoi(argv[1]);

    // 改变进程工作目录
    int ret = chdir(argv[2]);

    if (ret != 0)
    {
        std::cerr << "chdir error";
        exit(1);
    }
    // 启动epoll 监听
    epoll_run(port);
    return 0;
}