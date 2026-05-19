#pragma once
#include <string>
#include <sstream>
#include <memory>
#include <jsoncpp/json/json.h>
#include "logger.hpp"

namespace json_rpc {
    class JsonSerializer {
        public:
            // 序列化
            static bool serialize(const Json::Value& value, std::string& body){
                std::stringstream ss;
                Json::StreamWriterBuilder builder;
                std::unique_ptr<Json::StreamWriter> sw(builder.newStreamWriter());// 实例化工厂类对象
                int ret = sw->write(value, &ss);// 序列化到stringstream
                if(ret != 0){
                    LOG_ERROR("json序列化失败");
                    return false;
                }
                body = ss.str();
                return true;
            }
            // 反序列化
            static bool unserialize(const std::string &body, Json::Value &value) {
                Json::CharReaderBuilder crb;
                std::string errs;
                std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
                bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), &value, &errs);
                if (ret == false) { 
                    LOG_ERROR("json反序列化失败 : %s", errs.c_str());
                    return false;
                }
                return true;
            }
    };
}