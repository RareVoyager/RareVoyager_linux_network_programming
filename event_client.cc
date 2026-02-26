#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <arpa/inet.h>
#include <cstring>

#define SEVR_PORT 9526

void read_cb(struct bufferevent *bev, void *ctx)
{
    char buf[4096];
    while (true)
    {
        int n = bufferevent_read(bev, buf, sizeof(buf) - 1);
        //  n == 0 对端关闭 n < 0 出错
        if (n <= 0)
            break;
        // 添加结束标志
        buf[n] = '\0';
        std::cout << "[client] recv: " << buf << std::endl;
    }
}

void write_cb(struct bufferevent *bev, void *ctx)
{
    std::cout << "[client] write done" << std::endl;
}

void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    struct event_base *base = (struct event_base *)ctx;

    if (events & BEV_EVENT_CONNECTED)
    {
        std::cout << "[client] connected to server" << std::endl;

        const char *msg = "hello server\n";
        bufferevent_write(bev, msg, std::strlen(msg)); // ✅ strlen
        return;                                        // connected 不需要释放
    }

    if (events & BEV_EVENT_EOF)
    {
        std::cout << "[client] server closed (EOF)" << std::endl;
    }

    if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        std::cout << "[client] error: "
                  << evutil_socket_error_to_string(err)
                  << " (errno=" << err << ")" << std::endl;
    }

    if (events & BEV_EVENT_TIMEOUT)
    {
        std::cout << "[client] timeout" << std::endl;
    }

    bufferevent_free(bev);

    // 客户端一般在断线/错误后直接退出事件循环
    event_base_loopexit(base, nullptr);
}

void stdin_cb(evutil_socket_t fd, short events, void *ctx)
{
    struct bufferevent *bev = (struct bufferevent *)ctx;
    char buf[1024];
    int n = read(fd, buf, sizeof(buf));
    if (n > 0)
    {
        // 将标准输入的内容通过网络发送给服务器
        bufferevent_write(bev, buf, n);
    }
}

int main()
{
    // 1. 创建event_base
    struct event_base *base = event_base_new();

    // 2. 创建一个用跟服务器通信的bufferevent对象
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct bufferevent *bufevent = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    // 3. 连接服务器
    struct sockaddr_in sevr_addr;
    bzero(&sevr_addr, sizeof(sevr_addr));
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);
    inet_pton(AF_INET, "127.0.0.1", &sevr_addr.sin_addr.s_addr);
    bufferevent_socket_connect(bufevent, (struct sockaddr *)&sevr_addr, sizeof(sevr_addr));

    // 4. 设置回调函数
    bufferevent_setcb(bufevent, read_cb, write_cb, event_cb, base);

    // 启用读写
    bufferevent_enable(bufevent, EV_READ);

    // 监听标准输入输出
    struct event *ev_stdin = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, stdin_cb, bufevent);
    event_add(ev_stdin, 0);
    // 5. 开启事件循环
    event_base_dispatch(base);

    // 6. 函数结束前销毁
    bufferevent_free(bufevent);

    event_base_free(base);

    return 0;
}