#pragma once
#include <cstdio>
#include <ctime>

namespace json_rpc {
// 日志级别
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_DEFAULT_LEVEL LOG_LEVEL_INFO

// 日志宏定义
#define LOG(level, format, ...) \
     do { \
        if ((level) >= LOG_DEFAULT_LEVEL) { \
            time_t t = time(NULL); \
            struct tm lt; \
            localtime_r(&t, &lt); /* 线程安全版本 */ \
            char time_tmp[32] = {0}; \
            strftime(time_tmp, sizeof(time_tmp), "%m-%d %T", &lt); \
            printf("[%s][%s:%d] " format "\n", time_tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
        } \
    } while(0)


#define LOG_DEBUG(...) LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  LOG(LOG_LEVEL_INFO,  __VA_ARGS__)
#define LOG_WARN(...)  LOG(LOG_LEVEL_WARN,  __VA_ARGS__)
#define LOG_ERROR(...) LOG(LOG_LEVEL_ERROR, __VA_ARGS__)

}