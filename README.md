# RareVoyager Linux 网络编程学习日志
基于B站黑马Linux cpp教程学习 [黑马Linux网络编程](https://www.bilibili.com/video/BV1iJ411S7UA?t=0.7)

## 项目环境
Ubuntu 22.04
g++ 11.4.0
cmake 3.22.1

## 如何构建这个项目？

首先clone 项目
然后创建一个build 目录
然后执行cmake命令 构建可执行程序。具体指令如下
``` sh
mkdir build && cd build
cmake ..
make

```
紧接着在build 目录下生成了可执行文件。选择自己希望执行的执行即可
```sh
./test_service
```


**tcp_client.cc** 文件是客户端源文件
**tcp_service.cc** 文件是多进程版本源文件
**tcp_service_thread.cc** 是多线程版本
**select_service.cc** 文件是多路IO转接版本

`select` 优势是跨平台。缺点也很明显,最大承接1024个连接, 无法达到高并发。