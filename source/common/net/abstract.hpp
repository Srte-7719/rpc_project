#pragma once
#include "../base/fields.hpp"
#include <cstddef>
#include <memory>
#include <functional>
namespace json_rpc {
    //消息基类
    class BaseMessage{
        public:
        using ptr = std::shared_ptr<BaseMessage>;
         virtual ~BaseMessage() = default;
         //设置消息ID
         virtual void setId(const std::string& id) {
             _rid = id;
         }
         //获取消息ID
         virtual std::string rid()
         {
             return _rid;
         }

        //设置消息类型
        virtual void setMtype (MType mtype) {
            _mtype = mtype;
        }
        //获取消息类型
        virtual MType mtype()
        {
            return _mtype;
        }
        //序列化消息
        virtual std::string serialize() const= 0;
        //反序列化消息
        virtual bool unserialize(const std::string& msg) = 0;
        //检查消息是否有效
        virtual bool check() = 0;
        private:
         std::string _rid;//消息ID
         MType _mtype;//消息类型
    };

    //缓冲区基类
    class BaseBuffer{
        public:
        using ptr = std::shared_ptr<BaseBuffer>;
         virtual size_t readableSize() = 0;//可读字节数
         virtual int32_t peekInt32() = 0;//预读
         virtual void retrieveInt32() = 0;//删除32位整数
         virtual int32_t readInt32() = 0;//从缓存区取4字节数据并删掉
         virtual std::string retrieveAsString(size_t len) = 0;//从缓存区取所有数据并删掉
    };

    //协议基类
    class BaseProtocol{
        public:
        using ptr = std::shared_ptr<BaseProtocol>;
         virtual bool canProcessed(const BaseBuffer::ptr &buffer) = 0;//检查是否可以处理消息
         virtual bool onMessage(const BaseBuffer::ptr &buffer, BaseMessage::ptr & msg) = 0;//处理消息
         virtual std::string serialize(const BaseMessage::ptr& msg) = 0;//序列化消息
    };

    //连接基类
    class BaseConnection{
        public:
            using ptr = std::shared_ptr<BaseConnection>;
            virtual void send(const BaseMessage::ptr& msg) = 0;//发送消息
            virtual void shutdown() = 0;//关闭连接
            virtual bool connected() = 0;//检查是否已连接
        private:
            BaseProtocol::ptr _protocol;
    };


    using ConnectionCallback = std::function<void(BaseConnection::ptr)>;//连接回调
    using CloseCallback = std::function<void(BaseConnection::ptr)>;//关闭回调
    using MessageCallback = std::function<void(BaseConnection::ptr, BaseMessage::ptr&)>;//消息回调

    //服务器基类
    class BaseServer{
        public:
            using ptr = std::shared_ptr<BaseServer>;
            virtual void setConnectionCallback(ConnectionCallback cb) {
                _cb_connection = cb;
            }
            virtual void setCloseCallback(CloseCallback cb) {
                _cb_close = cb;
            }
            virtual void setMessageCallback(MessageCallback cb) {
                _cb_message = cb;
            }
            virtual void start() = 0;//启动服务器
        protected:
            ConnectionCallback _cb_connection;
            CloseCallback _cb_close;
            MessageCallback _cb_message;
    };

    //客户端
    class BaseClient{
        public:
            using ptr = std::shared_ptr<BaseClient>;
            virtual void setConnectionCallback(ConnectionCallback cb) {
                _cb_connection = cb;
            }//设置连接回调
            virtual void setCloseCallback(CloseCallback cb) {
                _cb_close = cb;
            }//设置关闭回调
            virtual void setMessageCallback(const MessageCallback& cb) {
                _cb_message = cb;
            }//设置消息回调
            virtual void connect() = 0;//连接服务器
            virtual void shutdown() = 0;//断开连接
            virtual bool send(const BaseMessage::ptr& msg) = 0;//发送消息
            virtual BaseConnection::ptr connection() = 0;//获取连接
            virtual bool connected() = 0;//检查是否已连接


        protected:
            ConnectionCallback _cb_connection;
            CloseCallback _cb_close;
            MessageCallback _cb_message;
    };
}
