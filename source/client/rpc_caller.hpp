#include "requestor.hpp"

namespace json_rpc {
    // 客户端调用器
    namespace client {
        class RpcCaller {
            public:
                using ptr = std::shared_ptr<RpcCaller>;
                using JsonAsyncResponse = std::future<Json::Value>;//异步响应
                using JsonResponseCallback = std::function<void(const Json::Value& rsp)>;//响应回调 
                RpcCaller(const Requestor::ptr &requestor): _requestor(requestor){}
                //同步调用，请求服务器响应，返回响应结果
                bool call(const BaseConnection::ptr &conn, const std::string &method,const Json::Value &params, Json::Value &result)
                {
                    LOG_INFO("开始同步rpc调用...");
                    //帮用户构造RpcRequest对象
                    auto req_msg = MessageFactory::create<RpcRequest>();
                    req_msg->setId(UUID::generate_uuid());//设置请求id
                    req_msg->setMtype(MType::REQ_RPC);//设置消息类型为rpc请求
                    req_msg->setMethod(method);//设置方法名
                    req_msg->setParams(params);//设置参数


                    //将请求消息发送给服务器，等待响应消息
                    BaseMessage::ptr rsp_msg;//响应消息
                    bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), rsp_msg);
                    if (ret == false){
                        LOG_ERROR("同步Rpc请求失败!");
                        return false;
                    }
                    LOG_INFO("收到响应，进行解析，获取结果!");

                    //将响应消息转换为RpcResponse对象
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(rsp_msg);
                    if (!rpc_rsp_msg) {
                        LOG_ERROR("rpc响应,向下类型转换失败!");
                        return false;
                    }
                    // 帮用户检查错误码
                    if (rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                    {
                        LOG_ERROR("Rpc调用失败,错误码:%d", (int)rpc_rsp_msg->rcode());
                        return false;
                    }
                    // 填充结果
                    result = rpc_rsp_msg->result();
                    return true;
                }

                //异步调用：返回future<Json::Value>
                bool call(const BaseConnection::ptr &conn, const std::string &method,const Json::Value &params,JsonAsyncResponse &result)
                {
                    LOG_INFO("开始异步rpc调用...");
                    //帮用户构造RpcRequest对象
                    auto req_msg = MessageFactory::create<RpcRequest>();
                    req_msg->setId(UUID::generate_uuid());//设置请求id
                    req_msg->setMtype(MType::REQ_RPC);//设置消息类型为rpc请求
                    req_msg->setMethod(method);//设置方法名
                    req_msg->setParams(params);//设置参数
                    //创建一个promise，用来保存Json结果
                    auto json_promise = std::make_shared<std::promise<Json::Value>>();
                    // 把promise对应的future返回给用户
                    result = json_promise->get_future();

                    // 绑定异步回调函数
                    Requestor::RequestCallback cb = std::bind(&RpcCaller::handleFutureResponse, this,  json_promise,std::placeholders::_1);
                    //将请求消息发送给服务器，等待响应消息
                    bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), cb);
                    if (ret == false){
                        LOG_ERROR("异步Rpc请求失败!");
                        return false;
                    }
                    LOG_INFO("异步RPC请求已发送,等待异步响应回调");
                    return true;
                }

                //回调调用：收到结果后自动调用用户的回调
                bool call(const BaseConnection::ptr &conn, const std::string &method,const Json::Value &params, JsonResponseCallback cb)
                {
                    auto req_msg = MessageFactory::create<RpcRequest>();
                    req_msg->setId(UUID::generate_uuid());//设置请求id
                    req_msg->setMtype(MType::REQ_RPC);//设置消息类型为rpc请求
                    req_msg->setMethod(method);//设置方法名
                    req_msg->setParams(params);//设置参数

                    Requestor::RequestCallback req_cb = std::bind(&RpcCaller::handleCallbackResponse, this,  cb,std::placeholders::_1);
                    //将请求消息发送给服务器，等待响应消息
                    bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), req_cb);
                    if (ret == false){
                        LOG_ERROR("回调Rpc请求失败!");
                        return false;
                    }
                    return true;
                }

            private:
                //回调函数模式的辅助回调函数
                void handleCallbackResponse(const JsonResponseCallback &cb,const BaseMessage::ptr &msg)
                {
                    //转成RpcResponse
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                    if (!rpc_rsp_msg) {
                        LOG_ERROR("rpc响应,向下类型转换失败!");
                        
                        return;
                    }
                    // 帮用户检查错误码
                    if (rpc_rsp_msg->rcode() != RCode::RCODE_OK) {
                        LOG_ERROR("Rpc调用失败,错误码:%d", (int)rpc_rsp_msg->rcode());
                        return;
                    }
                    // 填充结果
                    cb(rpc_rsp_msg->result());
                }
                
                
             //异步调用的辅助回调函数
                void  handleFutureResponse(std::shared_ptr<std::promise<Json::Value>> promise,const BaseMessage::ptr &msg)
                {
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                    if (!rpc_rsp_msg) {
                        LOG_ERROR("rpc响应,向下类型转换失败!");
                        promise->set_exception(std::make_exception_ptr(std::runtime_error("响应类型转换失败")));
                        return;
                    }
                    if (rpc_rsp_msg->rcode() != RCode::RCODE_OK) {
                        LOG_ERROR("Rpc调用失败,错误码:%d", (int)rpc_rsp_msg->rcode());
                        promise->set_exception(std::make_exception_ptr(std::runtime_error("RPC调用失败")));
                        return;
                    }
                    // 填充结果
                    promise->set_value(rpc_rsp_msg->result());
                }
                
                Requestor::ptr _requestor;//所有底层请求都交给它处理
        };
    }

}
