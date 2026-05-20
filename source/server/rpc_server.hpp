#include "../common/dispatcher/dispatcher.hpp"
#include "../client/rpc_client.hpp"
#include "rpc_router.hpp"
#include "rpc_registry.hpp"
#include "rpc_topic.hpp"

namespace json_rpc {
    namespace server {
        //注册中心服务端
        class RegistryServer {
            public:
                using ptr = std::shared_ptr<RegistryServer>;
                RegistryServer(int port):
                    _pd_manager(std::make_shared<ProviderDiscovererManager>()),
                    _dispatcher(std::make_shared<json_rpc::Dispatcher>())
                {
                    auto service_cb = std::bind(&ProviderDiscovererManager::onServiceRequest, _pd_manager.get(),
                        std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE, service_cb);

                    _server = json_rpc::ServerFactory::create(port);
                    auto message_cb = std::bind(&json_rpc::Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _server->setMessageCallback(message_cb);

                    auto close_cb = std::bind(&RegistryServer::onConnShutdown, this, std::placeholders::_1);
                    _server->setCloseCallback(close_cb);
                }
                void start() {
                    _server->start();
                }
            private:
                void onConnShutdown(const BaseConnection::ptr &conn) {
                    _pd_manager->onConnShutdown(conn);
                }
            private:
                ProviderDiscovererManager::ptr _pd_manager;
                Dispatcher::ptr _dispatcher;
                BaseServer::ptr _server;
        };

        //业务服务端,接收 RpcClient 发来的 RPC 调用请求同时自动向注册中心注册服务
        class RpcServer {
            public:
                using ptr = std::shared_ptr<RpcServer>;
                //对外地址, 是否启用注册, 注册中心地址

                RpcServer(const Address &access_addr, 
                    bool enableRegistry = false, 
                    const Address &registry_server_addr = Address()):
                    _enableRegistry(enableRegistry),
                    _access_addr(access_addr),
                    _router(std::make_shared<json_rpc::server::RpcRouter>()),
                    _dispatcher(std::make_shared<json_rpc::Dispatcher>())
                    {
                        //启用注册
                    if (enableRegistry) {
                        _reg_client = std::make_shared<client::RegistryClient>(
                            registry_server_addr.first, registry_server_addr.second);
                    }
                    //注册rpc方法
                    auto rpc_cb = std::bind(&RpcRouter::onRpcRequest, _router.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<json_rpc::RpcRequest>(json_rpc::MType::REQ_RPC, rpc_cb);
                    //创建rpc服务端
                    _server = json_rpc::ServerFactory::create(access_addr.second);
                    auto message_cb = std::bind(&json_rpc::Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _server->setMessageCallback(message_cb);
                }

                //注册业务方法（比如 add、login、pay）
                void registerMethod(const ServiceDescribe::ptr &service) {
                    if (_enableRegistry) {
                        _reg_client->registryMethod(service->method(), _access_addr);
                    }
                    _router->registerMethod(service);
                }
                void start() {
                    _server->start();
                }
            private:
                bool _enableRegistry;//是否启用服务注册
                Address _access_addr;//rpc服务提供端地址信息
                client::RegistryClient::ptr _reg_client;//服务注册客户端
                RpcRouter::ptr _router;
                Dispatcher::ptr _dispatcher;
                BaseServer::ptr _server;
        };

        class TopicServer {
            public:
                using ptr = std::shared_ptr<TopicServer>;
                TopicServer(int port):
                    _topic_manager(std::make_shared<TopicManager>()),
                    _dispatcher(std::make_shared<json_rpc::Dispatcher>())
                {
                    auto topic_cb = std::bind(&TopicManager::onTopicRequest, _topic_manager.get(),std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<TopicRequest>(MType::REQ_TOPIC, topic_cb);

                    _server = json_rpc::ServerFactory::create(port);//监听客户端连接
                    //收到客户端消息 → 交给分发器
                    auto message_cb = std::bind(&json_rpc::Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _server->setMessageCallback(message_cb);
                    //客户端断开连接 → 调用自己的onConnShutdown方法
                    auto close_cb = std::bind(&TopicServer::onConnShutdown, this, std::placeholders::_1);
                    _server->setCloseCallback(close_cb);
                }
                void start() {
                    _server->start();
                }
            private:
                void onConnShutdown(const BaseConnection::ptr &conn) {
                    _topic_manager->onShutdown(conn);
                }
            private:
                TopicManager::ptr _topic_manager;
                Dispatcher::ptr _dispatcher;
                BaseServer::ptr _server;
        };
    }
}