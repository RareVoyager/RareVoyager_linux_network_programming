cmake_minimum_required(VERSION 3.10)
project(servece LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 定义公共源文件变量 (修正了拼写，建议用复数命名习惯)
set(COMMON_SOURCES "wrap.cc")

# 2. 在生成可执行文件时，引用该变量 ${COMMON_SOURCES}
# 语法：add_executable(程序名 主文件 公共文件变量)

add_executable(tcp_client tcp_client.cc ${COMMON_SOURCES})

add_executable(test_service tcp_service.cc ${COMMON_SOURCES})