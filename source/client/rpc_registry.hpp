#pragma once
#include "requestor.hpp"
#include "source/common/message.hpp"
#include <unordered_set>

namespace json_rpc {
    namespace client {
        class Provider {
            public:
                using ptr = std::shared_ptr<Provider>;
                Provider(const Requestor::ptr &requestor) : _requestor(requestor){}//构造函数
                bool registryMethod(const BaseConnection::ptr &conn, const std::string &method, const Address &host)
                {
                     auto msg_req = MessageFactory::create<ServiceRequest>();
                     msg_req->setId(UUID::generate_uuid());
                     msg_req->setMtype(MType::REQ_SERVICE);
                     msg_req->setMethod(method);
                     msg_req->setHost(host);
                     msg_req->setServiceOptype(ServiceOptype::SERVICE_REGISTRY);

                     BaseMessage::ptr msg_rsp;//响应消息
                     bool ret = _requestor->send(conn, msg_req, msg_rsp);
                     if (ret == false) {
                        LOG_ERROR("%s 服务注册失败！", method.c_str());
                        return false;
                     }
                     auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                     if (service_rsp.get() == nullptr) {
                        LOG_ERROR("响应类型向下转换失败！");
                        return false;
                     }
                     if (service_rsp->rcode() != RCode::RCODE_OK) {
                        LOG_ERROR("服务注册失败，原因：%s", errReason(service_rsp->rcode()).c_str());
                        return false;
                     }
                     return true;
                }
            private:
            Requestor::ptr _requestor;//请求器
        };

        //方法主机映射
        class MethodHost{
            public:
                using ptr = std::shared_ptr<MethodHost>;
                MethodHost(): _idx(0){}//构造函数
                MethodHost(const std::vector<Address> &hosts) : _hosts(hosts.begin(), hosts.end()), _idx(0){}//构造函数
                void appendHost(const Address &host){
                    //中途收到了服务上线请求后被调用
                    std::unique_lock<std::mutex> lock(_mutex);
                    _hosts.push_back(host);
                }

                //删除主机
                void removeHost(const Address &host) {
                    //服务下线请求后被调用
                    std::unique_lock<std::mutex> lock(_mutex);
                    for (auto it = _hosts.begin(); it != _hosts.end(); ++it) {
                        if (*it == host) {
                            _hosts.erase(it);
                            break;
                        }
                    }
                }

                //选择主机
                Address chooseHost() {
                    //选择主机
                    std::unique_lock<std::mutex> lock(_mutex);
                    size_t pos = _idx++ % _hosts.size();
                    return _hosts[pos];//原子性不用_idx = (_idx + 1) % _hosts.size();
                }
                //判断是否为空
                bool empty() {
                    std::unique_lock<std::mutex> lock(_mutex);
                     return _hosts.empty();
                }
            private:
                std::mutex _mutex;
                size_t _idx;
                std::vector<Address> _hosts;//哈希没办法设置负载均衡，可能延迟删除？std::vector<Address> _hosts; +std::unordered_set<Address> _delete_set;
                
        };


        //服务发现器
        class Discoverer{
            public:
            using OfflineCallback = std::function<void(const Address&)>;
            using ptr = std::shared_ptr<Discoverer>;
            Discoverer(const Requestor::ptr &requestor, const OfflineCallback &cb) : _requestor(requestor), _offline_callback(cb){}
            //服务发现
            bool serviceDiscovery(const BaseConnection::ptr &conn, const std::string &method, Address &host) {
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    //根据方法名查找主机
                    auto it = _method_hosts.find(method);
                    if (it != _method_hosts.end()) {
                        if (it->second->empty() == false) {
                            host = it->second->chooseHost();
                            return true;
                        }
                    }
                }

                //当前服务的提供者为空
                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->setId(UUID::generate_uuid());
                msg_req->setMtype(MType::REQ_SERVICE);
                msg_req->setMethod(method);
                msg_req->setServiceOptype(ServiceOptype::SERVICE_DISCOVERY);
                BaseMessage::ptr msg_rsp;
                bool ret = _requestor->send(conn, msg_req, msg_rsp);

                 if (ret == false) {
                    LOG_ERROR("服务发现失败！");
                    return false;
                 }

                 
                 auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                 if (!service_rsp) {
                    LOG_ERROR("服务发现失败！响应类型转换失败！");
                    return false;
                 }
                 if (service_rsp->rcode() != RCode::RCODE_OK) {
                    LOG_ERROR("服务发现失败，原因：%s", errReason(service_rsp->rcode()).c_str());
                    return false;
                 }

                 //当前无对应的服务提供主机
                 std::unique_lock<std::mutex> lock(_mutex);
                 auto method_host = std::make_shared<MethodHost>(service_rsp->Hosts());
                 if (method_host->empty()) {
                    LOG_ERROR("%s 服务发现失败！没有能够提供服务的主机！", method.c_str());
                    return false;
                 }
                  host = method_host->chooseHost();
                 _method_hosts[method] = method_host;
                return true;
            }

            void onServiceRequest(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg) {
                 //判断上线还是下线，如果都不是那就不用处理了
                auto optype = msg->serviceOptype();
                 std::string method = msg->method();
                 std::unique_lock<std::mutex> lock(_mutex);
                  if (optype == ServiceOptype::SERVICE_ONLINE){
                    //服务上线请求
                    auto it = _method_hosts.find(method);
                    if (it == _method_hosts.end()) {
                         auto method_host = std::make_shared<MethodHost>();
                         method_host->appendHost(msg->host());
                         _method_hosts[method] = method_host;
                    }
                    else{
                        it->second->appendHost(msg->host());
                    }
                 }
                 else if (optype == ServiceOptype::SERVICE_OFFLINE){
                    //服务下线请求
                    auto it = _method_hosts.find(method);
                    // 找到了删
                    if (it != _method_hosts.end()) {
                        it->second->removeHost(msg->host());
                        _offline_callback(msg->host());
                    }
                    else{
                        std::cout << "服务下线失败！没有找到该服务的主机！\n";
                    }
                 }
            }
            private:
                std::mutex _mutex;
                OfflineCallback _offline_callback;//下线回调
                std::unordered_map<std::string, MethodHost::ptr> _method_hosts;//方法主机映射
                Requestor::ptr _requestor;
        };
    }
}