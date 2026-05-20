#pragma once
#include "../common/net/net.hpp"
#include "../common/net/message.hpp"
#include "../common/base/uuid.hpp"
#include "../common/base/logger.hpp"
#include <json/json.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <mutex>
namespace  json_rpc {
    namespace server {

        //提供提供者管理器
        class ProviderManager
        {
                public:
                    using ptr = std::shared_ptr<ProviderManager>;
                    //单个服务端的描述
                    struct Provider
                    {
                        using ptr = std::shared_ptr<Provider>;
                        std::mutex _mutex;//保护conn
                        BaseConnection::ptr conn;//和注册中心的连接
                        Address host;
                        std::vector<std::string> methods;//add/sub/login
                        Provider(const BaseConnection::ptr &c, const Address &h):conn(c), host(h){}
                        
                        void appendMethod(const std::string &method){//添加支持的方法
                            std::unique_lock<std::mutex> lock(_mutex);
                            methods.push_back(method);
                        }
                    };
                    void addProvider(const BaseConnection::ptr &c, const Address &h, const std::string &method)//添加提供提供者
                    {
                        Provider::ptr provider;
                        //查找关联的服务提供者，找到就获取找不到创建
                        {
                            std::unique_lock<std::mutex> lock(_mutex);
                            auto it = _conns.find(c);
                            if (it != _conns.end()) 
                            {
                                provider = it->second;
                            }else {
                                provider = std::make_shared<Provider>(c, h);
                                _conns.insert(std::make_pair(c, provider));
                            }
                        }
                        //把这个方法加入列表，_providers新增数据
                        auto &providers = _providers[method];
                        providers.insert(provider);
                        //向着服务对象里面新增一个能提供的服务名称
                        provider->appendMethod(method);
                    }
                    void delProvider(const BaseConnection::ptr &c){
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(c);
                        if (it == _conns.end()) {
                            return;
                        }
                        for (auto & method : it->second->methods){
                            auto &providers = _providers[method];
                            providers.erase(it->second);
                        }
                        _conns.erase(it); //删除连接与服务提供者的关联关系
                    }
                    //根据方法获取提供提供者的主机列表
                    std::vector<Address> methodHosts(const std::string &method) {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _providers.find(method);
                        if (it == _providers.end()) {
                            return {};
                        }
                        std::vector<Address> hosts;
                        for (auto &provider : it->second) {
                            hosts.push_back(provider->host);
                        }
                        return hosts;
                    }
                    Provider::ptr getProvider(const BaseConnection::ptr &c){
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(c);
                        if (it != _conns.end()) {
                           return it->second;
                        }
                         return Provider::ptr();
                    }//当服务提供者连接关闭时，获取他的信息
                private:
                    std::mutex _mutex;//保护_providers
                    std::unordered_map<std::string, std::set<Provider::ptr>> _providers;//提供提供者
                    std::unordered_map<BaseConnection::ptr, Provider::ptr> _conns;//连接-提供提供者映射
        };

        //服务发现
        class DiscovererManager {
            public:
            using ptr = std::shared_ptr<DiscovererManager>;
            struct Discoverer {
                using ptr = std::shared_ptr<Discoverer>;
                std::mutex _mutex;
                BaseConnection::ptr conn;//发现者关联的客户端连接
                std::vector<std::string> methods;//发现过的服务名称
                Discoverer(const BaseConnection::ptr &c) : conn(c){}
                void appendMethod(const std::string &method) {//添加发现过的服务名称
                    std::unique_lock<std::mutex> lock(_mutex);
                    methods.push_back(method);
                }
            };

            Discoverer::ptr addDiscoverer(const BaseConnection::ptr &c, const std::string &method)//服务发现的时候新增发现者，新增服务名称
            {
                Discoverer::ptr discoverer;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(c);
                    if (it != _conns.end()) {
                        discoverer = it->second;
                    }else {
                        discoverer = std::make_shared<Discoverer>(c);
                        _conns.insert(std::make_pair(c, discoverer));
                    }
                    auto &discoverers = _discoverers[method];
                    discoverers.insert(discoverer);
                }
                discoverer->appendMethod(method);
                return discoverer;
            }
            void delDiscoverer(const BaseConnection::ptr &c)//删除发现者
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _conns.find(c);
                if (it == _conns.end()) {
                    return;
                }
                for (auto &method : it->second->methods) {
                    auto& discoverers = _discoverers[method];
                    discoverers.erase(it->second);
                }
                _conns.erase(it); //删除连接与发现者的关联关系

            }
            void onlineNotify(const std::string &method, const Address &host)//通知发现者服务提供者上线
            {
                return notify(method, host, ServiceOptype::SERVICE_ONLINE);
            }
            void offlineNotify(const std::string &method, const Address &host)//通知发现者服务提供者下线
            {
                return notify(method, host, ServiceOptype::SERVICE_OFFLINE);
            }


            private:
            //当某个服务上线/下线时通知订阅了这个服务的客户端
                void notify(const std::string &method, const Address &host, ServiceOptype optype) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _discoverers.find(method);
                    if (it == _discoverers.end()) {
                        return;
                    }
                    //构造通知消息
                    auto msg_req = MessageFactory::create<ServiceRequest>();
                    msg_req->setId(UUID::generate_uuid());
                    msg_req->setMethod(method);
                    msg_req->setHost(host);
                    msg_req->setServiceOptype(optype);
                    for (auto &discoverer : it->second) {
                        discoverer->conn->send(msg_req);
                    }
                }
                std::mutex _mutex;//保护_discoverers
                std::unordered_map<std::string, std::set<Discoverer::ptr>> _discoverers;//发现者
                std::unordered_map<BaseConnection::ptr, Discoverer::ptr> _conns;//连接-发现者映射
        };

        //服务发现管理器
        class ProviderDiscovererManager {
            public:
            using ptr = std::shared_ptr<ProviderDiscovererManager>;
            ProviderDiscovererManager() : _providers(std::make_shared<ProviderManager>()), _discoverers(std::make_shared<DiscovererManager>()){}
            void onServiceRequest(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg)//处理服务发现请求
            {
                //服务注册/服务发现
                ServiceOptype optype = msg->serviceOptype();
                if (optype == ServiceOptype::SERVICE_REGISTRY){
                    //服务注册
                    LOG_INFO("%s:%d 注册服务 %s", msg->host().first.c_str(), msg->host().second, msg->method().c_str());
                    _providers->addProvider(conn, msg->host(), msg->method());
                    _discoverers->onlineNotify(msg->method(), msg->host());
                    return registryResponse(conn, msg);
                }else if (optype == ServiceOptype::SERVICE_DISCOVERY){
                    //服务发现
                    LOG_INFO("客户端要进行 %s 服务发现！", msg->method().c_str());
                    _discoverers->addDiscoverer(conn, msg->method());
                    return discoveryResponse(conn, msg);
                }else {
                    LOG_ERROR("收到服务操作请求，但是操作类型错误！");
                    return errorResponse(conn, msg);
                }
            }
            void onConnShutdown(const BaseConnection::ptr &conn)//处理连接关闭
            {
                auto provider = _providers->getProvider(conn);
                if (provider != nullptr) {
                    LOG_INFO("%s:%d 服务下线", provider->host.first.c_str(), provider->host.second);
                    for (auto &method : provider->methods){
                        _discoverers->offlineNotify(method, provider->host);
                    }
                    _providers->delProvider(conn);
                }
                _discoverers->delDiscoverer(conn);
            }
            private:
            //发送错误响应
                void errorResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg) {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setId(msg->rid());
                    msg_rsp->setMtype(MType::RSP_SERVICE);
                    msg_rsp->setRcode(RCode::RCODE_INVALID_OPTYPE);
                    msg_rsp->setOptype(ServiceOptype::SERVICE_UNKNOW);
                    conn->send(msg_rsp);
                }
            //发送服务注册响应
                void registryResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg) {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setId(msg->rid());
                    msg_rsp->setMtype(MType::RSP_SERVICE);
                    msg_rsp->setRcode(RCode::RCODE_OK);
                    msg_rsp->setOptype(ServiceOptype::SERVICE_REGISTRY);
                    conn->send(msg_rsp);
                }
            //发送服务发现响应
                void discoveryResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg) {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setId(msg->rid());
                    msg_rsp->setMtype(MType::RSP_SERVICE);
                    msg_rsp->setOptype(ServiceOptype::SERVICE_DISCOVERY);
                    std::vector<Address> hosts = _providers->methodHosts(msg->method());
                    if (hosts.empty()) {
                        msg_rsp->setRcode(RCode::RCODE_NOT_FOUND_SERVICE);
                        msg_rsp->setMethod(msg->method());//否则客户端校验失败
                        msg_rsp->setHost(std::vector<Address>());
                        return conn->send(msg_rsp);
                    }
                    msg_rsp->setRcode(RCode::RCODE_OK);
                    msg_rsp->setMethod(msg->method());
                    msg_rsp->setHost(hosts);
                    return conn->send(msg_rsp);
                }
                ProviderManager::ptr _providers;//提供提供者管理器
                DiscovererManager::ptr _discoverers;//发现者管理器
        };
    };
    
}