cmake_minimum_required(VERSION 3.10)
project(servece LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 定义公共源文件变量 (修正了拼写，建议用复数命名习惯)
set(COMMON_SOURCES "wrap.cc")
# 2. 在生成可执行文件时，引用该变量 ${COMMON_SOURCES}
# 语法：add_executable(程序名 主文件 公共文件变量)

add_executable(tcp_client tcp_client.cc ${COMMON_SOURCES})

add_executable(test_service tcp_service.cc ${COMMON_SOURCES})

add_executable(tcp_service_thread tcp_service_thread.cc ${COMMON_SOURCES})

add_executable(tcp_service tcp_service.cc ${COMMON_SOURCES})

add_executable(select_service select_service.cc ${COMMON_SOURCES})

add_executable(test_array_service test_array_service.cc ${COMMON_SOURCES})

add_executable(epoll_service epoll_service.cc ${COMMON_SOURCES})

add_executable(udp_client udp_client.cc ${COMMON_SOURCES})

add_executable(udp_service udp_service.cc ${COMMON_SOURCES})

add_executable(read_fifo_event libevent_read_fifo.cc ${COMMON_SOURCES})
target_link_libraries(read_fifo_event PRIVATE event)

add_executable(write_fifo_event libevent_write_fifo.cc ${COMMON_SOURCES})
target_link_libraries(write_fifo_event PRIVATE event)

add_executable(event_service event_service.cc ${COMMON_SOURCES})
target_link_libraries(event_service PRIVATE event)

add_executable(event_client event_client.cc ${COMMON_SOURCES})
target_link_libraries(event_client PRIVATE event)