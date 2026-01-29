#include <iostream>
#include <event2/event.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "wrap.h"

void write_cb(evutil_socket_t fd, short what, void *arg)
{
    char buf[] = "hello libevent";
    write(fd, buf, sizeof(buf) + 1);
    sleep(1);
}
int main()
{
    int fd = open("testfifo", O_WRONLY | O_NONBLOCK);
    if (fd == -1)
    {
        Wrap::Die("open filded");
    }

    struct event_base *base = event_base_new();

    // 创建事件

    struct event *ev = nullptr;
    ev = event_new(base, fd, EV_WRITE | EV_PERSIST, write_cb, nullptr);
    // 添加事件到base
    event_add(ev, NULL);
    // 启动循环

    event_base_dispatch(base);
    event_base_free(base);
    close(fd);
    return 0;
}