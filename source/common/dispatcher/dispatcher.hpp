#pragma once
#include "../net.hpp"
#include "../message.hpp"

namespace json_rpc
{
    class Callback{
        public:
            using ptr = std::shared_ptr<Callback>;
            //处理消息
            virtual void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) = 0;
    };

    template<typename T>
    //具体类型的消息回调函数
    //用户注册的业务处理函数，必须是这个类型的对象
    class CallbackT : public Callback{
        public:
            using ptr = std::shared_ptr<CallbackT<T>>;
            //处理消息,连接对象 + 具体类型的消息
            using MessageCallback = std::function<void(const BaseConnection::ptr &conn, std::shared_ptr<T> &msg)>;
            //构造函数保存用户传来的业务处理函数
            CallbackT(const MessageCallback &handler):_handler(handler){}
            void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) override {
                auto type_msg = std::dynamic_pointer_cast<T>(msg);
                _handler(conn, type_msg);
            }        
            private:
            //存储用户的业务回调函数
            MessageCallback _handler;
    };

    //分发器
    class Dispatcher{
        public:
            using ptr = std::shared_ptr<Dispatcher>;
            template<typename T>
            void registerHandler(MType mtype, const typename CallbackT<T>::MessageCallback &handler) {
                std::unique_lock<std::mutex> lock(_mutex);
                auto cb = std::make_shared<CallbackT<T>>(handler);
                _handlers.insert(std::make_pair(mtype, cb));
            }

            //分发消息
            void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) {
                //找到消息类型对应的业务处理函数，进行调用即可
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _handlers.find(msg->mtype());
                if (it != _handlers.end()) {
                    return it->second->onMessage(conn, msg);
                }
                //没有找到指定类型的处理回调--因为客户端和服务端都是我们自己设计的，因此不可能出现这种情况
                LOG_ERROR("收到未知类型的消息: %d", msg->mtype());
                conn->shutdown();
            }
        private:
            std::mutex _mutex;// 互斥锁，保护哈希表
            // 存储消息类型 → 回调对象的哈希表
            std::unordered_map<MType, Callback::ptr> _handlers;
    };
}
