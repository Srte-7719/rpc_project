# CMake 最低版本
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(dict_server)

# C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# 头文件路径（你自己的环境）
include_directories(../../build/release-install-cpp11/include)

# 库文件路径
link_directories(../../build/release-install-cpp11/lib)

# 生成可执行文件
add_executable(dict_server server.cpp)

# 链接 muduo 库 + pthread
target_link_libraries(dict_server
    muduo_net
    muduo_base
    pthread
)