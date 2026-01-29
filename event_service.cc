#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <strings.h>

#define SEVR_PORT 9526

void read_cb(struct bufferevent *bev, void *ctx)
{
    char buf[BUFSIZ];
    // 获取实际读取到的字节数
    int n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    if (n > 0)
    {
        buf[n] = '\0'; // 必须手动截断，保证 cout 安全
        std::cout << "[Server] recv from client: " << buf << std::flush;

        const char *ret = "I'm server, I already received your information\n";
        // 使用 strlen 确保发送完整字符串
        bufferevent_write(bev, ret, ::strlen(ret));
    }
}

void write_cb(struct bufferevent *bev, void *ctx)
{
    std::cout << "[server] write_cb " << std::endl;
}

void event_cb(struct bufferevent *bev, short events, void *ctx)
{
    // 1) 对端正常关闭（TCP FIN），或者你读到 0
    if (events & BEV_EVENT_EOF)
    {
        std::cout << "[event_cb] client disconnected (EOF)" << std::endl;
    }

    // 2) 发生错误（例如对端强制关闭、网络错误等）
    if (events & BEV_EVENT_ERROR)
    {
        // 从 bufferevent 中取出更详细的错误信息（如果有）
        int err = EVUTIL_SOCKET_ERROR();
        std::cout << "[event_cb] error: "
                  << err
                  << " (errno=" << err << ")" << std::endl;
    }

    // 3) 读/写超时（需要你对 bev 设置超时才会触发）
    if (events & BEV_EVENT_TIMEOUT)
    {
        std::cout << "[event_cb] timeout" << std::endl;
    }

    // 4) （一般用于客户端）连接建立成功事件；服务端通常不会用到
    if (events & BEV_EVENT_CONNECTED)
    {
        std::cout << "[event_cb] connected" << std::endl;
        return; // connected 不需要释放
    }

    // 5) 释放 bufferevent（你创建时用了 BEV_OPT_CLOSE_ON_FREE，所以会顺带 close(fd)）
    bufferevent_free(bev);
}

void listener_callback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ptr)
{
    // 3. 处理连接后的操作(创建新的fd)
    std::cout << "connect new client..." << std::endl;
    // 4. 回调函数被调用，说明有一个新客户端连接上来。会得到一个新fd，用于跟客户端通信（读、写）。
    struct event_base *base = (struct event_base *)ptr;
    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    // 设置回调函数
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

    // 读缓冲开启
    bufferevent_enable(bev, EV_READ);
}
int main()
{
    // 1.创建event_base 底座
    struct event_base *base = event_base_new();

    // 2. 创建服务器监听

    struct sockaddr_in sevr_addr;
    bzero(&sevr_addr, sizeof(sevr_addr));
    sevr_addr.sin_family = AF_INET;
    sevr_addr.sin_port = htons(SEVR_PORT);
    sevr_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener *listen = evconnlistener_new_bind(base, listener_callback, base, LEV_OPT_REUSEABLE, 128, (struct sockaddr *)&sevr_addr, sizeof(sevr_addr));

    event_base_dispatch(base);
    evconnlistener_free(listen);
    event_base_free(base);
    return 0;
}