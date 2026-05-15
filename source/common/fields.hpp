#pragma once
#include <string>
#include <unordered_map>

namespace json_rpc {
    //请求字段
    #define KEY_METHOD "method"//方法名
    #define KEY_PARAMS "parameters"//参数
    #define KEY_TOPIC_KEY "topic_key"//主题键
    #define KEY_TOPIC_MSG "topic_msg"//主题消息
    #define KEY_OPTYPE "optype"//操作类型
    #define KEY_HOST "host"//主机名
    #define KEY_HOST_IP "ip"//主机IP
    #define KEY_HOST_PORT "port"//主机端口
    //响应字段
    #define KEY_RCODE "rcode"//响应码
    #define KEY_RESULT "result"//结果


    //消息类型
    enum class MType{
        REQ_RPC = 0,//请求消息
        RSP_RPC,//响应消息
        REQ_TOPIC,//请求主题消息
        RSP_TOPIC,//响应主题消息
        RSP_SERVICE,//响应服务消息
        REQ_SERVICE,//请求服务消息
    };

    //响应码
    enum class RCode{
        RCODE_OK = 0,
        RCODE_PARSE_FAILED,//解析失败
        RCODE_ERROR_MSGTYPE,//消息类型错误
        RCODE_INVALID_MSG,//无效消息
        RCODE_DISCONNECTED,//已断开连接
        RCODE_INVALID_PARAMS,//无效参数
        RCODE_NOT_FOUND_SERVICE,//未找到服务
        RCODE_INVALID_OPTYPE,//无效操作类型
        RCODE_NOT_FOUND_TOPIC,//未找到主题
        RCODE_INTERNAL_ERROR,//内部错误
    };
    //请求类型
    enum class RType{
        REQ_ASYNC = 0,//异步请求
        REQ_CALLBACK,//回调请求
    };
    //主题操作类型
    enum class TopicOptype {
        TOPIC_CREATE = 0,//创建主题
        TOPIC_REMOVE,//删除主题
        TOPIC_SUBSCRIBE,//订阅主题
        TOPIC_CANCEL,//取消订阅主题
        TOPIC_PUBLISH,//发布主题
    };
    //服务操作类型
    enum class ServiceOptype {
        SERVICE_REGISTER = 0,//注册服务
        SERVICE_DISCOVER,//发现服务
        SERVICE_ONLINE,//服务上线
        SERVICE_OFFLINE,//服务下线
    };


    static std::string errReason(RCode code){
        static std::unordered_map<RCode,std::string> err_map = {
            {RCode::RCODE_OK, "成功处理！"},
            {RCode::RCODE_PARSE_FAILED, "解析失败！"},
            {RCode::RCODE_ERROR_MSGTYPE, "消息类型错误！"},
            {RCode::RCODE_INVALID_MSG, "无效消息！"},
            {RCode::RCODE_DISCONNECTED, "已断开连接！"},
            {RCode::RCODE_INVALID_PARAMS, "无效参数！"},
            {RCode::RCODE_NOT_FOUND_SERVICE, "未找到服务！"},
            {RCode::RCODE_INVALID_OPTYPE, "无效操作类型！"},
            {RCode::RCODE_NOT_FOUND_TOPIC, "未找到主题！"},
            {RCode::RCODE_INTERNAL_ERROR, "内部错误！"},
        };
        auto it = err_map.find(code);
        if(it == err_map.end()){
            return "未知错误！";
        }
        return it->second;
    }
}
