# RareVoyager Linux 网络编程学习日志
基于B站黑马Linux cpp教程学习 [黑马Linux网络编程](https://www.bilibili.com/video/BV1iJ411S7UA?t=0.7)

## 项目环境
Ubuntu 22.04
g++ 11.4.0
cmake 3.22.1

## 如何构建这个项目？


首先clone 项目
需要先下载三方库 libevent。这里使用的是libevent-2.1.8-stable
具体执行如下
```sh
# 先拉取代码
git clone https://github.com/libevent/libevent.git

# 进入到目录
cd libevent

# 切换分支
git checkout release-2.1.8-stable

# 验证一下
git branch

---------------------也可以一步到位--------------------
git clone -b release-2.1.8-stable https://github.com/libevent/libevent.git

```

下载完成之后, 安装到usr/local/include目录下
```sh
./autogen.sh
./configure
make -j4
sudo make install
```
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
`poll` 没有解决select 的缺点
`epoll` 解决了select 的缺点
`libevent` 高性能的三方库

最后的web服务器我也只搭建了一个基础的框架,相关代码在web_service中。原因是相关知识已经基本学过, 这里更像是跟着复习顺便学习Html知识。

具体的基础知识可以看博客内容
[回声之境](https://www.rarevoyager.top/)

