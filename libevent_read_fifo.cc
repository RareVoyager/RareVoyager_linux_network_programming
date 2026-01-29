#include <iostream>
#include <event2/event.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "wrap.h"
#include <event2/event_struct.h>

void read_cb(evutil_socket_t fd, short what, void *arg)
{
    char buf[BUFSIZ];
    int len = read(fd, buf, sizeof(buf));

    std::cout << "read from write " << buf << std::endl;
    sleep(1);
}
int main()
{
    unlink("testfifo");
    mkfifo("testfifo", 0644);
    int fd = open("testfifo", O_RDONLY | O_NONBLOCK);
    if (fd == -1)
    {
        Wrap::Die("open filded");
    }

    struct event_base *base = event_base_new();

    // 创建事件

    struct event *ev = nullptr;
    ev = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, nullptr);
    // 添加事件到base
    event_add(ev, NULL);
    // 启动循环

    event_base_dispatch(base);
    event_base_free(base);
    close(fd);
    return 0;
}