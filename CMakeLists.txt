cmake_minimum_required(VERSION 3.10)
project(rpc_project)

# ===================== 基础配置 =====================
# C++标准（你的代码用了shared_ptr、模板完美转发，C++11足够）
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # 生成编译命令，方便IDE识别
# 编译选项：开启所有警告+调试信息+无优化（适合测试）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O0")

# ===================== Muduo 依赖配置 =====================
# Muduo安装路径（和你写的保持一致）
set(MUDUO_ROOT /home/jinglei/rpc_project/build/release-install-cpp11)

# 头文件路径：包含项目根目录、source目录、Muduo头文件
# 关键！加了${PROJECT_SOURCE_DIR}/source，这样你的cpp文件里#include "message.hpp"能直接找到
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/source  # 适配你的头文件都在source/目录下
    ${MUDUO_ROOT}/include
)

# 库文件路径：告诉CMake去哪里找Muduo的.so文件
link_directories(
    ${MUDUO_ROOT}/lib
)

# ===================== 生成可执行文件 =====================
# 1. RPC服务器（对应source/test_server.cpp）
add_executable(rpc_server source/test_server.cpp)
# 链接依赖库：muduo网络库、基础库、线程库、jsoncpp
target_link_libraries(rpc_server 
    muduo_net 
    muduo_base 
    pthread 
    jsoncpp
)

# 2. RPC客户端（对应source/test_client.cpp）
add_executable(rpc_client source/test_client.cpp)
target_link_libraries(rpc_client 
    muduo_net 
    muduo_base 
    pthread 
    jsoncpp
)