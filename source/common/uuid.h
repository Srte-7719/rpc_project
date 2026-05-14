

#pragma once
#include <cstddef>
#include <string>
#include <random>
#include <sstream>// 字符串流
#include <iomanip>// 格式化输出
#include <atomic>// 原子操作
#include "logger.h"

namespace json_rpc {
    class UUID {
        public:
            // 生成uuid
            static std::string generate_uuid(){
                //生成一个唯一、不重复、格式标准、长度固定的UUID字符串
                std::stringstream ss;
                //1.构造机器随机数对象
                std::random_device rd;
                //2.构造伪随机数对象
                std::mt19937 gen(rd());//用真随机数做种子，生成高质量、速度快、周期极长的伪随机数
                //3.构造限定数据范围对象
                std::uniform_int_distribution<int> distribution(0, 255);// 限定数据范围
                //4.生成8个随机数，按照特定格式组织为16进制字符串
                for (int i = 0; i < 8; ++i) {
                    if (i == 4 || i == 6) ss << "-";
                    ss << std::hex << std::setw(2) << std::setfill('0') << distribution(gen);
                }
                //a7f292c1-a73b-de01
                ss << "-";
                //5.定义一个8字节随机数，逐字节组织成为16进制数字字符的字符串
                static std::atomic<size_t> seq(1);
                size_t cur = seq.fetch_add(1);// 原子操作，获取当前值并增加1
                for (int i = 7; i >= 0; i--) {
                    if (i == 5) ss << "-";
                    ss << std::setw(2)<<std::setfill('0')<<std::hex<<((cur >> (i*8)) & 0xFF);// 逐字节组织成为16进制字符串a7f292c1-a73b-de01-000000000001
                }
                return ss.str();
            }
    };

}