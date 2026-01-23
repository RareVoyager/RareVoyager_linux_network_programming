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
