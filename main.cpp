// #include <iostream>
// #include <unistd.h>
// #include <sys/epoll.h>
// #include <fcntl.h>

// int main()
// {
//     // 1. 创建 epoll 实例
//     int epfd = epoll_create(1);

//     // 2. 设置 stdin 为非阻塞（epoll 推荐）
//     // 首先获取了stdin的内核状态标志
//     int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
//     // 原有标志基础上添加了非阻塞标志，并让内核以这个标志来工作
//     fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
//     // 3. 注册 stdin
//     epoll_event ev;
//     ev.events = EPOLLIN;
//     ev.data.fd = STDIN_FILENO;
//     epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

//     epoll_event events[10];

//     while (true)
//     {
//         // 4. 等待事件
//         int nfds = epoll_wait(epfd, events, 10, -1);

//         for (int i = 0; i < nfds; ++i)
//         {
//             if (events[i].data.fd == STDIN_FILENO)
//             {
//                 std::string line;
//                 std::getline(std::cin, line);
//                 std::cout << "epoll read: " << line << std::endl;
//             }
//         }
//     }
//     return 0;
// }

#include <arpa/inet.h>
#include <iostream>

extern


int main()
{
    const char *str = "192.168.10.130";

    struct in_addr addr; // ✅ 实际内存

    int ret = inet_pton(AF_INET, str, &addr);
    if (ret == 1)
    {
        std::cout << "success" << std::endl;
        std::cout << addr.s_addr << std::endl;
    }
    else if (ret == 0)
    {
        std::cout << "invalid address format" << std::endl;
    }
    else
    {
        perror("inet_pton");
    }
    return 0;
}
