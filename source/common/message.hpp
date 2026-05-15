#pragma once
#include "abstract.hpp"
#include "fields.hpp"
#include "json.h"
#include "logger.h"
#include <vector>

namespace json_rpc {
        typedef std::pair<std::string, int> Address;

  
    class JsonMessage : public BaseMessage{
        public:
            using ptr = std::shared_ptr<JsonMessage>;
            virtual std::string serialize() const override
            {
                std::string body;
                bool ret = JsonSerializer::serialize(_body, body);
                if(ret == false)    
                {
                    return std::string();
                }
                return body;
            }
            virtual bool unserialize(const std::string& msg) override
            {
                return JsonSerializer::unserialize(msg, _body);
            }
        protected:
            Json::Value _body;
    };


    class JsonRequest: public JsonMessage{
        public:
            using ptr = std::shared_ptr<JsonRequest>;
    };


    class JsonResponse: public JsonMessage{
        public:
            using ptr = std::shared_ptr<JsonResponse>;
            virtual bool check() override
            {
                //判断响应状态码是否存在
                if(_body[KEY_RCODE].isNull() == true)
                {

                    LOG_ERROR("没有响应状态码");
                    return false;
                }
                if (_body[KEY_RCODE].isIntegral() == false)
                {
                    LOG_ERROR("响应状态码类型错误");
                    return false;
                }
                return true;
            }
             //获取响应码
           virtual RCode rcode() const
            {
                return static_cast<RCode>(_body[KEY_RCODE].asInt());
            }
            //设置响应码
            virtual void setRcode(const RCode code) 
            {
                _body[KEY_RCODE] = (int)code;
            }
    };


    //Rpc请求
    class RpcRequest: public JsonRequest{
        public:
            using ptr = std::shared_ptr<RpcRequest>;
            virtual bool check() override //检查Rpc请求字段检测
            {
                if(_body[KEY_METHOD].isNull() == true || _body[KEY_METHOD].isString() == false)
                {
                    LOG_ERROR("Rpc请求中没有名字/方法类型错误");
                    return false;
                }
                if(_body[KEY_PARAMS].isNull() == true || _body[KEY_PARAMS].isObject() == false)
                {
                    LOG_ERROR("Rpc请求中没有参数/参数类型错误");
                    return false;
                }
                return true;
            }
            //获取方法名
            std::string getMethod() const
            {
                return _body[KEY_METHOD].asString();
            }
            //设置方法名 
            void setMethod(const std::string& method_name)
            {
                _body[KEY_METHOD] = method_name;
            }
            //获取参数
            Json::Value getParams() const
            {
                return _body[KEY_PARAMS];
            }
            //设置参数
            void setParams(const Json::Value& params)
            {
                _body[KEY_PARAMS] = params;
            }
    };

//主题请求
    class TopicRequest: public JsonRequest{
        public:
            using ptr = std::shared_ptr<TopicRequest>;
            virtual bool check() override //检查Topic请求字段检测
            {
                if(_body[KEY_TOPIC_KEY].isNull() == true || _body[KEY_TOPIC_KEY].isString() == false)
                {
                    LOG_ERROR("主题请求中没有名字/主题类型错误");
                    return false;
                }
                //操作类型
                if(_body[KEY_OPTYPE].isNull() == true || _body[KEY_OPTYPE].isIntegral() == false)
                {
                    LOG_ERROR("主题请求中没有操作类型/操作类型类型错误");
                    return false;
                }
                if(_body[KEY_OPTYPE].asInt() == (int)TopicOptype::TOPIC_PUBLISH && 
                    (_body[KEY_TOPIC_MSG].isNull() == true||
                    _body[KEY_TOPIC_MSG].isString() == false))
                {
                    LOG_ERROR("主题消息发布请求中没有消息内容字段或者消息内容类型错误");
                    return false;
                }
                return true;
            }
            //获取主题名称
            std::string topicKey()
            {
                return _body[KEY_TOPIC_KEY].asString();
            }
            //设置主题名称
            void setTopicKey(const std::string& topic_key)
            {
                _body[KEY_TOPIC_KEY] = topic_key;
            }
            //获取操作类型
            TopicOptype topicOptype()
            {
                return static_cast<TopicOptype>(_body[KEY_OPTYPE].asInt());
            }
            //设置操作类型
            void setTopicOptype(TopicOptype optype)
            {
                _body[KEY_OPTYPE] = (int)optype;
            }

            //获取主题消息
            std::string topicMsg()
            {
                return _body[KEY_TOPIC_MSG].asString();
            }
            //设置主题消息
            void setTopicMsg(const std::string &msg)
            {
                _body[KEY_TOPIC_MSG] = msg;
            }
        
    };


    //服务请求
    class ServiceRequest: public JsonRequest{
        public:
            using ptr = std::shared_ptr<ServiceRequest>;
            virtual bool check() override //检查Service请求字段检测
            {
                if(_body[KEY_METHOD].isNull() == true || _body[KEY_METHOD].isString() == false)
                {
                    LOG_ERROR("服务请求中没有名字/方法类型错误");
                    return false;
                }
               if(_body[KEY_OPTYPE].isNull() == true || _body[KEY_OPTYPE].isIntegral() == false)
                {
                    LOG_ERROR("服务请求中没有操作类型/操作类型类型错误");
                    return false;
                }
                

                if (_body[KEY_OPTYPE].asInt() == (int)ServiceOptype::SERVICE_REGISTER)
                {
                     //主机信息
                if(_body[KEY_HOST].isNull() == true || _body[KEY_HOST].isObject() == false)
                {
                    LOG_ERROR("服务请求中没有主机信息/主机信息类型错误");
                    return false;
                }
                if(_body[KEY_HOST][KEY_HOST_IP].isNull() == true || _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
                   _body[KEY_HOST][KEY_HOST_PORT].isNull() == true || _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false)
                {
                    LOG_ERROR("服务请求中没有主机IP或主机端口类型错误");
                    return false;
                }
                }
               
                return true;
            }
            //获取方法名
            std::string method() const
            {
                return _body[KEY_METHOD].asString();
            }
            //设置方法名 
            void setMethod(const std::string& method_name)
            {
                _body[KEY_METHOD] = method_name;
            }
            //获取操作类型
            ServiceOptype serviceOptype()
            {
                return (ServiceOptype)_body[KEY_OPTYPE].asInt();
            }
            //设置操作类型
            void setServiceOptype(ServiceOptype optype)
            {
                _body[KEY_OPTYPE] = (int)optype;
            }
            //获取主机信息
            Address host()
            {
                Address addr;
                addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
                addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
                return addr;
            }
            //设置主机信息
            void setHost(const Address &addr)
            {
               Json::Value val;
               val[KEY_HOST_IP] = addr.first;
               val[KEY_HOST_PORT] = addr.second;
               _body[KEY_HOST] = val;
            }
            
    };


    //------------response----------------

    //Rpc响应
    class RpcResponse: public JsonResponse{
        public:
            using ptr = std::shared_ptr<RpcResponse>;
        virtual bool check() override //检查Rpc响应字段检测
            {
                if(_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
                {
                    LOG_ERROR("Rpc响应中没有响应码/响应码类型错误");
                    return false;
                }
                if(_body[KEY_RESULT].isNull()) {
                    LOG_ERROR("Rpc响应中没有结果字段");
                    return false;
                }
                return true;
            }
            //获取结果
            Json::Value result()
            {
                return _body[KEY_RESULT];
            }
            //设置结果
            void setResult(const Json::Value &result)
            {
                _body[KEY_RESULT] = result;
            }
    };

    //主题响应
    class TopicResponse: public JsonResponse{
        public:
            using ptr = std::shared_ptr<TopicResponse>; 

    };

    //服务响应
   class ServiceResponse: public JsonResponse{
        public:
            using ptr = std::shared_ptr<ServiceResponse>;
        virtual bool check() override //检查Service响应字段检测
            {
                if(_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
                {
                    LOG_ERROR("Service响应中没有响应码/响应码类型错误");
                    return false;
                }
                if(_body[KEY_OPTYPE].isNull() == true || _body[KEY_OPTYPE].isIntegral() == false)
                {
                    LOG_ERROR("Service响应中没有操作类型/操作类型类型错误");
                    return false;
                }
                // 检查发现服务响应字段检测
                if (_body[KEY_OPTYPE].asInt() == static_cast<int>(ServiceOptype::SERVICE_DISCOVER)) 
                {
                  if (_body[KEY_METHOD].isNull() || !_body[KEY_METHOD].isString()) 
                  {
                    LOG_ERROR("Service响应中没有方法名或方法名类型错误");
                    return false;
                  }
                  if (_body[KEY_HOST].isNull() || !_body[KEY_HOST].isArray()) 
                  {
                    LOG_ERROR("Service响应中主机信息不是数组类型");
                    return false;
                  }
                }
                return true;
            }
            //获取方法名
            std::string method()
            {
                return _body[KEY_METHOD].asString();
            }
            //设置方法名
            void setMethod(const std::string& method_name)
            {
                _body[KEY_METHOD] = method_name;
            }
            //设置主机信息
            void setHost(std::vector<Address>& addrs)
            {
                _body[KEY_HOST] = Json::Value(Json::arrayValue);//先清空为新数组
                for(auto addr: addrs)//遍历主机信息
                {
                    Json::Value val;
                    val[KEY_HOST_IP] = addr.first;//主机IP
                    val[KEY_HOST_PORT] = addr.second;//主机端口
                    _body[KEY_HOST].append(val);//添加主机信息
                }
            }
            //获取主机信息
            std::vector<Address> Hosts()
            {
                std::vector<Address> addrs;
                int sz = _body[KEY_HOST].size();//主机信息数量
                for(int i = 0; i < sz; i++)
                {
                    Address addr;
                    addr.first = _body[KEY_HOST][i][KEY_HOST_IP].asString();//主机IP
                    addr.second = _body[KEY_HOST][i][KEY_HOST_PORT].asInt();//主机端口
                    addrs.push_back(addr);//添加主机信息
                }
                return addrs;
            }
    };

    //实现一个消息对象的生产工厂
    class MessageFactory{
        public:
            static BaseMessage::ptr create(MType mtype)
            {
                switch(mtype)
                {
                    case MType::REQ_RPC:
                        return std::make_shared<RpcRequest>();//Rpc请求
                    case MType::RSP_RPC:
                        return std::make_shared<RpcResponse>();//Rpc响应
                    case MType::REQ_TOPIC:
                        return std::make_shared<TopicRequest>();//主题请求
                    case MType::RSP_TOPIC:
                        return std::make_shared<TopicResponse>();//主题响应
                    case MType::RSP_SERVICE:
                        return std::make_shared<ServiceResponse>();//服务响应
                    case MType::REQ_SERVICE:            
                        return std::make_shared<ServiceRequest>();//服务请求
                    default:
                        LOG_ERROR("未知的消息类型");
                        return nullptr;
                }
                return BaseMessage::ptr();
            }

            template<typename T,typename ...Args>
           static std::shared_ptr<T> create(Args&& ...args)
           {
                return std::make_shared<T>(std::forward<Args>(args)...);//完美转发，创建消息对象
           }

            
    };

};
