#include "../common/dispatcher/dispatcher.hpp"
#include "requestor.hpp"
#include "rpc_caller.hpp"
#include "rpc_registry.hpp"
#include "rpc_topic.hpp"
//客户端封装
namespace json_rpc {
    namespace client {
        //注册客户端
        class RegistryClient {
            public:
                using ptr = std::shared_ptr<RegistryClient>;
                RegistryClient(const std::string &ip, int port):
                _requestor(std::make_shared<Requestor>()),
                _provider(std::make_shared<client::Provider>(_requestor)),
                _dispatcher(std::make_shared<Dispatcher>()) 
                {

                    auto rsp_cb = std::bind(&client::Requestor::onResponse, _requestor.get(), std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE, rsp_cb);

                    auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), std::placeholders::_1, std::placeholders::_2);

                    _client = ClientFactory::create(ip, port);
                    _client->setMessageCallback(message_cb);
                    _client->connect();
                }

                 //对提供的服务注册接口
                bool registryMethod(const std::string &method, const Address &host) {
                    return _provider->registryMethod(_client->connection(), method, host);
                }
            private:
                Requestor::ptr _requestor;
                client::Provider::ptr _provider;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;

        };
        //服务发现客户端
        class DiscoveryClient {
            public:
                using ptr = std::shared_ptr<DiscoveryClient>;

                DiscoveryClient(const std::string &ip, int port, const Discoverer::OfflineCallback &cb): 
                _requestor(std::make_shared<Requestor>()),_discoverer(std::make_shared<client::Discoverer>(_requestor, cb)),_dispatcher(std::make_shared<Dispatcher>())
                {
                    auto rsp_cb = std::bind(&client::Requestor::onResponse, _requestor.get(), std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE, rsp_cb);
                    auto req_cb = std::bind(&client::Discoverer::onServiceRequest, _discoverer.get(),std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE, req_cb);
                    auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(),std::placeholders::_1, std::placeholders::_2);
                    _client = ClientFactory::create(ip, port);
                    _client->setMessageCallback(message_cb);
                    _client->connect();
                }
                //向外提供的服务发现接口
                bool serviceDiscovery(const std::string &method, Address &host){
                    return _discoverer->serviceDiscovery(_client->connection(), method, host);
                }
            private:
                Requestor::ptr _requestor;
                client::Discoverer::ptr _discoverer;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;
        };

        //RPC客户端
        class RpcClient {
            public:
                using ptr = std::shared_ptr<RpcClient>;
                //1.初始化三大核心
                 RpcClient(bool enableDiscovery, const std::string &ip, int port):
                  _enableDiscovery(enableDiscovery),
                  _requestor(std::make_shared<Requestor>()),
                  _dispatcher(std::make_shared<Dispatcher>()),
                  _caller(std::make_shared<json_rpc::client::RpcCaller>(_requestor))
                   {

                    //2.注册RPC响应回调
                    auto rsp_cb = std::bind(&client::Requestor::onResponse, _requestor.get(), std::placeholders::_1, std::placeholders::_2);
                     _dispatcher->registerHandler<BaseMessage>(MType::RSP_RPC, rsp_cb);

                    //两种初始化连接
                    //1.启用服务发现 → ip:port 是注册中心地址
                    if (_enableDiscovery) {
                        auto offline_cb = std::bind(&RpcClient::delClient, this, std::placeholders::_1);
                        _discovery_client = std::make_shared<DiscoveryClient>(ip, port, offline_cb);
                    }else {
                        // 不启用服务发现 → ip:port 是服务提供者地址
                        auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                            std::placeholders::_1, std::placeholders::_2);
                        _rpc_client = ClientFactory::create(ip, port);// 创建网络客户端
                        _rpc_client->setMessageCallback(message_cb);// 设置消息回调
                        _rpc_client->connect();
                    }
                  }
                  // 1. 同步阻塞调用
                 bool call(const std::string &method, const Json::Value &params, Json::Value &result) {
                    //获取服务提供者
                    BaseClient::ptr client = getClient(method);
                    if (client.get() == nullptr) {
                        return false;
                    }
                    //通过客户端连接，发送rpc请求
                    return _caller->call(client->connection(), method, params, result);
                }
                // 2. 异步Future调用
                bool call(const std::string &method, const Json::Value &params, RpcCaller::JsonAsyncResponse &result) {
                    BaseClient::ptr client = getClient(method);
                    if (client.get() == nullptr) {
                        return false;
                    }
                    return _caller->call(client->connection(), method, params, result);
                }
                // 3. 异步回调调用
                bool call(const std::string &method, const Json::Value &params, const RpcCaller::JsonResponseCallback &cb) {
                    BaseClient::ptr client = getClient(method);
                    if (client.get() == nullptr) {
                        return false;
                    }
                    return _caller->call(client->connection(), method, params, cb);
                }
            private:
                // 创建新客户端,连接服务端，加入连接池
                BaseClient::ptr newClient(const Address &host) {
                    auto client = ClientFactory::create(host.first, host.second);
                    auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                    std::placeholders::_1, std::placeholders::_2);
                    client->setMessageCallback(message_cb);
                    client->connect();
                    if (!client->connected()) {
                        LOG_ERROR("无法连接到服务提供者 %s:%d", host.first.c_str(), host.second);
                        return nullptr;
                    }
                    putClient(host, client);
                    return client;
                }
                //连接池核心 → 客户端管理
                BaseClient::ptr getClient(const Address &host) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _rpc_clients.find(host);
                    if (it == _rpc_clients.end()) {
                        return BaseClient::ptr();
                    }
                    return it->second;
                }
                BaseClient::ptr getClient(const std::string method) {
                    BaseClient::ptr client;
                    
                    if (_enableDiscovery) {
                        //1. 通过服务发现，获取服务提供者地址信息
                        Address host;
                        bool ret = _discovery_client->serviceDiscovery(method, host);
                        if (ret == false) {
                            LOG_ERROR("当前 %s 服务，没有找到服务提供者！", method.c_str());
                            return BaseClient::ptr();
                        }
                        //2. 查看服务提供者是否已有实例化客户端，有则直接使用，没有则创建
                        client = getClient(host);
                        if (!client) {
                            client = newClient(host);
                            if (!client) {
                                LOG_ERROR("创建到 %s:%d 的客户端失败", host.first.c_str(), host.second);
                                return nullptr;
                            }
                        }
                        if (client.get() == nullptr) {//没有找打已实例化的客户端，则创建
                            client = newClient(host);
                        }
                    }else {
                        client = _rpc_client;
                    }
                    return client;
                }
                void putClient(const Address &host, BaseClient::ptr &client) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _rpc_clients.insert(std::make_pair(host, client));
                }
                void delClient(const Address &host) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _rpc_clients.erase(host);
                }
            private:
                struct AddressHash {
                    size_t operator()(const Address &host) const{
                        std::string addr = host.first + std::to_string(host.second);
                        return std::hash<std::string>{}(addr);
                    }
                };
                bool _enableDiscovery;// 服务发现开关
                //服务发现客户端
                DiscoveryClient::ptr _discovery_client;// 连接注册中心，查询服务地址
                 //RPC通信核心发请求、收响应、匹配请求-响应
                Requestor::ptr _requestor;// 管理所有RPC请求，匹请求→响应
                RpcCaller::ptr _caller;// RPC调用封装器：序列化参数、发送请求、解析结果
                Dispatcher::ptr _dispatcher;

                BaseClient::ptr _rpc_client;//客户端连接
                std::mutex _mutex;
                
                //<"127.0.0.1:8080", client1>
                std::unordered_map<Address, BaseClient::ptr, AddressHash> _rpc_clients;//这里用的是长连接，用于服务发现的客户端连接池
                //这里用的是长连接，开一个连接池
        };


        class TopicClient {
            public:
                TopicClient(const std::string &ip, int port):
                    _requestor(std::make_shared<Requestor>()),
                    _dispatcher(std::make_shared<Dispatcher>()),
                    _topic_manager(std::make_shared<TopicManager>(_requestor)) {
                    
                    auto rsp_cb = std::bind(&Requestor::onResponse, _requestor.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_TOPIC, rsp_cb);

                    auto msg_cb = std::bind(&TopicManager::onPublish, _topic_manager.get(),
                        std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<TopicRequest>(MType::REQ_TOPIC, msg_cb);
                    
                    auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _rpc_client = ClientFactory::create(ip, port);
                    _rpc_client->setMessageCallback(message_cb);
                    _rpc_client->connect();
                }
                bool create(const std::string &key) {
                    return _topic_manager->create(_rpc_client->connection(), key);
                }
                bool remove(const std::string &key) {
                    return _topic_manager->remove(_rpc_client->connection(), key);
                }
                bool subscribe(const std::string &key, const TopicManager::SubCallback &cb) {
                    return _topic_manager->subscribe(_rpc_client->connection(), key, cb);
                }
                bool cancel(const std::string &key) {
                    return _topic_manager->cancel(_rpc_client->connection(), key);
                }
                bool publish(const std::string &key, const std::string &msg) {
                    return _topic_manager->publish(_rpc_client->connection(), key, msg);
                }
                void shutdown() {
                    _rpc_client->shutdown();
                }
            private:
                Requestor::ptr _requestor;
                TopicManager::ptr _topic_manager;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _rpc_client;//用于未启用服务发现
        };
    }
}