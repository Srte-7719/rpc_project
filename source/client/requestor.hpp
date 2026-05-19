#pragma once
#include "../common/net.hpp"
#include "../common/message.hpp"
#include "../common/base/uuid.hpp"
#include "source/common/abstract.hpp"
#include "source/common/fields.hpp"
#include <future>
#include <functional>

namespace json_rpc {
    // 客户端请求发送器
    namespace client {
        //请求发送器
        //负责发送请求并等待响应
        class Requestor{
            public:
              // 请求描述
              // 描述一个请求，包括请求消息、返回值类型、异步调用响应、异步调用回调函数
              struct RequestDescribe {
                using ptr = std::shared_ptr<RequestDescribe>;
                BaseMessage::ptr request;                // 请求消息
                RType rtype;                             // 返回值类型
                std::promise<BaseMessage::ptr> response; // 异步调用响应
                std::function<void(const BaseMessage::ptr &)>callback; // 异步调用回调函数
              };
                using ptr = std::shared_ptr<Requestor>;//请求发送器指针
                using RequestCallback = std::function<void(const BaseMessage::ptr&)>;//异步调用的回调函数
                using AsyncResponse = std::future<BaseMessage::ptr>;//异步调用的响应

            //处理响应
            //根据 rid 查找请求描述，设置响应
            void onResponse(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) {
               std::string rid = msg->rid();//从响应里取出rid
                RequestDescribe::ptr rdp = getDescribe(rid);//根据rid查找对应的请求描述
                if (!rdp) {
                    LOG_ERROR("收到响应 - %s，但是未找到对应的请求！", rid.c_str());
                    return;
                }
                if (rdp->rtype == RType::REQ_ASYNC) {
                    rdp->response.set_value(msg);//设置异步调用响应
                
                } 
                else if (rdp->rtype == RType::REQ_CALLBACK) {
                    //回调调用，调用回调函数
                    if (rdp->callback) {
                        rdp->callback(msg);
                    }
                }
                else {
                    LOG_ERROR("未知的请求类型！");
                }
                delDescribe(rid);//删除请求描述
            }

            //发送请求
            //返回值是异步调用的响应
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, AsyncResponse &async_rsp) {
                RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_ASYNC);
                if (!rdp) {
                    LOG_ERROR("创建请求描述失败！");
                    return false;
                }
                conn->send(req);
                async_rsp = rdp->response.get_future();//获取异步调用响应
                return true;
            }

            //发送请求，等待响应
            //阻塞等待异步调用响应
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, BaseMessage::ptr &rsp) {
                AsyncResponse rsp_future;
                bool ret = send(conn, req, rsp_future);
                if (!ret) {
                    return false;
                }
                rsp = rsp_future.get();//阻塞等待future的结果获取异步调用响应
                return true;
            }

            //发送请求，等待响应
            //阻塞等待异步调用响应
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, const RequestCallback &cb) {
                RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_CALLBACK, cb);// 创建请求描述，类型是回调
                if (!rdp) {
                    LOG_ERROR("创建请求描述失败！");
                    return false;
                }
                conn->send(req);
                return true;
            }


            private:

            //创建请求描述
              RequestDescribe::ptr newDescribe(const BaseMessage::ptr &req, RType rtype, const RequestCallback &cb = RequestCallback())
              {
                std::unique_lock<std::mutex> lock(_mutex);
                 RequestDescribe::ptr rd = std::make_shared<RequestDescribe>();
                 rd->request = req;
                 rd->rtype = rtype;

                 //回调调用，保存回调函数
                if (rtype == RType::REQ_CALLBACK && cb) {
                    rd->callback = cb;
                }
                // 存入哈希表
                _request_desc.insert(std::make_pair(req->rid(), rd));
                 return rd;
            }

            //根据 rid 查找请求描述
               RequestDescribe::ptr getDescribe(const std::string &rid) 
               {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _request_desc.find(rid);
                if (it == _request_desc.end()) {
                    return RequestDescribe::ptr();
                }
                return it->second;
            } 
            //根据 rid 删除请求描述
            void delDescribe(const std::string &rid) {
                std::unique_lock<std::mutex> lock(_mutex);
                _request_desc.erase(rid);
            }


            private:
                std::mutex _mutex;
                std::unordered_map<std::string, RequestDescribe::ptr> _request_desc;

            };
        }
}