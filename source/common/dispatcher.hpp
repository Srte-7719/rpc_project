#pragma once
#include "net.hpp"
#include "message.hpp"

namespace json_rpc
{
    class Callback{
        public:
            using ptr = std::shared_ptr<Callback>;
            //处理消息
            virtual void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) = 0;
            virtual ~Callback() = default;
    };

    template<typename T>
    class CallbackT : public Callback{
        public:
            using ptr = std::shared_ptr<CallbackT<T>>;
            //处理消息,连接对象 + 具体类型的消息
            using MessageCallback = std::function<void(const BaseConnection::ptr &conn, std::shared_ptr<T> &msg)>;
            //构造函数保存用户传来的业务处理函数
            CallbackT(const MessageCallback &handler):handler_(handler){

            }
            virtual void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) override{
                auto type_msg = std::dynamic_pointer_cast<T>(msg);//通用的BaseMessage基类指针，安全地转成具体的消息类型指针
                 _handler(conn, type_msg);
            }        
            private:
            //存储用户的业务回调函数
            MessageCallback handler_;
    };

    //分发器
    class Dispatcher{
        public:
            using ptr = std::shared_ptr<Dispatcher>;
            template<typename T>
            void registerHandler(MType mtype, const typename CallbackT<T>::MessageCallback &handler) {
                std::lock_guard<std::mutex> lock(_mutex);//线程安全
                 // 把用户的业务函数，封装成 CallbackT<T> 对象
                auto cb = std::make_shared<CallbackT<T>>(handler);
                //存入哈希表：消息类型 → 回调对象
                _handlers.insert(std::make_pair(mtype, cb));
            }

            void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto it = _handlers.find(msg->mtype());
                if (it != _handlers.end()) {
                    it->second->onMessage(conn, msg);
                    return;
                }
                // 如果没有找到匹配的回调，直接返回
                LOG_ERROR("收到未知类型的消息：%d", static_cast<int>(msg->mtype()));
                conn->shutdown();//关闭连接
            }
        private:
            std::mutex _mutex;// 互斥锁，保护哈希表
            // 存储消息类型 → 回调对象的哈希表
            std::unordered_map<MType, Callback::ptr> _handlers;
    };
}
