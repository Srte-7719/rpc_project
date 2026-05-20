#pragma once
#include "base/logger.hpp"
#include "abstract.hpp"
#include "fields.hpp"
#include "message.hpp"
#include <cstddef>// 定义size_t类型
#include <linux/limits.h>// 限制文件路径长度
#include <memory>
#include <muduo/net/Callbacks.h>
#include <muduo/net/EventLoop.h> // 事件循环
#include <muduo/net//TcpConnection.h> // TCP连接封装
#include <muduo/net/Buffer.h>// 网络缓冲区
#include <muduo/net/TcpServer.h>// TCP服务器
#include <muduo/net/TcpClient.h>// TCP客户端
#include <muduo/net/EventLoopThread.h>// 事件循环线程
#include <muduo/base/CountDownLatch.h>// 计数器
#include <string>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace json_rpc
{
   //muduo缓冲区类，用于封装muduo库的缓冲区
    class MuduoBuffer: public BaseBuffer{
        public:
         using ptr = std::shared_ptr<MuduoBuffer>;
         MuduoBuffer(muduo::net::Buffer* buffer) : _buffer(buffer) 
         {}

         //查看可读数据的字节数
         virtual size_t readableSize() override {
            return _buffer->readableBytes();
         }

         //查看32位整数
         virtual int32_t peekInt32() override {
            //muduo库是一个网络库，从缓存区取出一个4字节整形，会进行网络字节序的转换
            //这里返回的是主机字节序的整数
            return _buffer->peekInt32();
         }
         //删除32位整数
         virtual void retrieveInt32() override {
            return _buffer->retrieveInt32();
         }

         //从缓存区取4字节数据并删掉
         virtual int32_t readInt32() override {
            return _buffer->readInt32();
         }

         //从缓存区取所有数据并删掉
         virtual std::string retrieveAsString(size_t len) override {
            return _buffer->retrieveAsString(len);
         }

         private:
          muduo::net::Buffer *_buffer;//对象是别人创建的，我们只是持有指针*
    };


    //缓冲区工厂类，用于创建缓冲区对象
    class BufferFactory{
      public:
      template<typename ...Args>
      static BaseBuffer::ptr create(Args&& ...args) {
        return std::make_shared<MuduoBuffer>(std::forward<Args>(args)...);
    }
};


    //网络协议类，实现网络通信协议
     class LVProtocol: public BaseProtocol{
        public:
        using ptr = std::shared_ptr<LVProtocol>;
         
        //检查是否可以处理消息
        virtual bool canProcessed(const BaseBuffer::ptr &buffer) override {
            //检查是否还有足够的数据
            if(buffer->readableSize() < (lenFieldsLength)) {
                return false;
            }
            int32_t total_len = buffer->peekInt32();
            if (buffer->readableSize() < (total_len)) {
                    return false;
            }
            return true;
         }//检查是否可以处理消息

         //处理消息
         virtual bool onMessage(const BaseBuffer::ptr &buffer, BaseMessage::ptr & msg) override {
            //调用onMessage，默认足够
            int32_t total_len = buffer->readInt32();//读取消消息长度
            //读取消息类型
            MType mytype = (MType)buffer->readInt32();
            //读取消息ID长度
            int32_t idlen = buffer->readInt32();
            //读取消消息ID
            std::string id = buffer->retrieveAsString(idlen);
            //读取消消息体
           int32_t bodylen = total_len - (lenFieldsLength + mtypeFieldsLength + idtypeFieldsLength + idlen);
           //长度不合法直接断开 
           if (bodylen < 0 || bodylen > static_cast<int32_t>(_max_data_size)) {
               LOG_ERROR("协议错误bodylen = %d", bodylen);
               return false;
            } 
           
           std::string body = buffer->retrieveAsString(bodylen);

            msg = MessageFactory::create(mytype);
            if(msg.get() == nullptr) {
                LOG_ERROR("消息类型错误，构建消息对象失败");
                return false;
            }
            //反序列化消息体
            bool ret = msg->unserialize(body);
            if(ret == false) {
                LOG_ERROR("消息反序列化失败");
                return false;
            }
            msg->setId(id);
            msg->setMtype(mytype);


             if (!msg->check()) 
             {
               LOG_ERROR("消息字段校验失败");
               return false;
            }
            return true;
         }

         //序列化消息
         virtual std::string serialize(const BaseMessage::ptr& msg) override {
            //取出消息
            std::string body = msg->serialize();
            std::string id = msg->rid();//获取消息ID
            int32_t mtype_net = htonl((int32_t)msg->mtype());//转换为网络字节序的整数//消息类型字段长度
            int32_t idlen_net = htonl(id.size());//转换为网络字节序的整数//消息ID长度字段长度
            
           int32_t total_len = lenFieldsLength + mtypeFieldsLength + idtypeFieldsLength + id.size() + body.size();
            //转换为网络字节序的整数,消息总字段长度
            int32_t total_len_net = htonl(total_len);   // 用于网络传输的字段
            
            std::string result;
            result.reserve(total_len); 
            result.append((char*)&total_len_net, lenFieldsLength);//消息总字段长度
            result.append((char*)&mtype_net, mtypeFieldsLength);//消息类型字段长度
            result.append((char*)&idlen_net, idtypeFieldsLength);//消息ID长度字段长度
            result.append(id);//消息ID
            result.append(body);//消息体
            return result;
         }

         private:
         // len | mytype |idlen |id | body网络通信协议格式
            const size_t lenFieldsLength = 4;//消息长度字段长度
            const size_t mtypeFieldsLength = 4;//消息类型字段长度
            const size_t idtypeFieldsLength = 4;//消息ID长度字段长度
            const size_t _max_data_size = (1 << 16);
    };

    //协议工厂类
    class ProtocolFactory {
    public:
      template <typename... Args>
      static BaseProtocol::ptr create(Args &&...args) {
        return std::make_shared<LVProtocol>(std::forward<Args>(args)...);
      }
    };

   class MuduoConnection: public BaseConnection{
        public:
            using ptr = std::shared_ptr<MuduoConnection>;
            //构造函数
            MuduoConnection(const muduo::net::TcpConnectionPtr &conn,const BaseProtocol::ptr &protocol)
             : _protocol(protocol), _conn(conn) {
            }
            //发送消息
            virtual void send(const BaseMessage::ptr& msg) override{
               std::string body = _protocol->serialize(msg);
               _conn->send(body);
            }
            virtual void shutdown() override{//关闭连接
               _conn->shutdown();
            }
            virtual bool connected() override{//检查是否已连接
               return _conn->connected();
            }

         private:
            BaseProtocol::ptr _protocol;//协议对象
            muduo::net::TcpConnectionPtr _conn;//连接对象
    };


    //连接工厂类
 class ConnectionFactory {
public:
    template<typename ...Args>
    static BaseConnection::ptr create(Args&& ...args) {
        return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
    }
};

     class MuduoServer: public BaseServer{
        public:
            using ptr = std::shared_ptr<MuduoServer>;
            //构造函数
            MuduoServer(int port) : 
            _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port), 
                "MuduoServer", muduo::net::TcpServer::kReusePort),
            _protocol(ProtocolFactory::create()){}
            virtual void start() {
                _server.setConnectionCallback(std::bind(&MuduoServer::onConnection, this, std::placeholders::_1));
                _server.setMessageCallback(std::bind(&MuduoServer::onMessage, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _server.start();//开始监听
                _baseloop.loop();//开始死循环事件监控
            }

         private:
         void onConnection(const muduo::net::TcpConnectionPtr &conn)
         {
            if(conn->connected())
            {
                std::cout << "连接建立！" << std::endl;
                auto muduo_conn = ConnectionFactory::create(conn,_protocol);
                {
                  std::lock_guard<std::mutex> lock(_mutex); 
                  _connections.insert(std::make_pair(conn, muduo_conn));
                }
                 if (_cb_connection) _cb_connection(muduo_conn);
            }
            else
            {
                std::cout << "连接关闭！" << std::endl;
                BaseConnection::ptr muduo_conn;
                {
                  std::lock_guard<std::mutex> lock(_mutex); 
                  auto it = _connections.find(conn);
                  if(it == _connections.end())
                  {
                     return; // 连接不存在，直接返回
                  }
                  muduo_conn = it->second;
                  _connections.erase(it);
                }

                if(_cb_close)
                {
                  LOG_DEBUG("正在调用关闭回调函数");
                  _cb_close(muduo_conn);
                }//调用关闭回调函数
            }
         }


         //消息回调函数
         void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp receiveTime)
            {
               LOG_INFO("连接有数据到来，开始处理！");
              auto base_buf = BufferFactory::create(buf);//创建缓冲区对象
              while(1)
              {
               if(_protocol->canProcessed(base_buf) == false)
               {
                  //数据不足
                  if(base_buf->readableSize() > _max_data_size)
                  {
                    conn->shutdown();//关闭连接
                    LOG_ERROR("连接关闭缓冲区数据大小超过最大消息大小");
                     return ;
                  }
                 break;
               }
               //数据足够，解析消息
               BaseMessage::ptr msg;
               bool ret = _protocol->onMessage(base_buf, msg);
               if(ret == false)
               {
                  conn->shutdown();
                  LOG_ERROR("缓存区数据错误");
                  return ;
               }
               // 消息反序列化成功
               BaseConnection::ptr base_conn;
               {
                 std::unique_lock<std::mutex> lock(_mutex);
                 auto it = _connections.find(conn);
                 if (it == _connections.end()) {
                   conn->shutdown();
                   return;
                 }
                 base_conn = it->second;
               }
               // 调用回调函数进行消息处理
               if (_cb_message)
                 _cb_message(base_conn, msg);
            }
         }
         
         private:
            const size_t _max_data_size = (1<<16);//最大消息大小
            BaseProtocol::ptr _protocol;//协议对象
            muduo::net::EventLoop _baseloop;
            muduo::net::TcpServer _server;
            std::mutex _mutex;
            std::unordered_map<muduo::net::TcpConnectionPtr, BaseConnection::ptr> _connections;
    };

    //服务器工厂类
    class ServerFactory {
    public:
      template <typename... Args>
      static BaseServer::ptr create(Args &&...args) {
        return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
      }
    };

       class MuduoClient : public BaseClient{
        public:
            using ptr = std::shared_ptr<MuduoClient>;
            //构造函数
            MuduoClient(const std::string &sip, int sport):
                _protocol(ProtocolFactory::create()),
                _baseloop(_loopthread.startLoop()),
                _downlatch(1),
                _connect_success(false),
                _client(_baseloop, muduo::net::InetAddress(sip, sport), "MuduoClient"){}

            virtual void connect() override 
            {
               LOG_INFO("设置回调函数，连接服务器");
               //设置连接事件（连接建立/管理）的回调
               _client.setConnectionCallback(std::bind(&MuduoClient::onConnection, this, std::placeholders::_1));
               //设置连接消息的回调
               _client.setMessageCallback(std::bind(&MuduoClient::onMessage, this, 
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
               //连接服务器
               _client.connect();
               _downlatch.wait();
               if (!_connect_success) {
                 LOG_ERROR("连接服务器失败！");
                 _conn.reset();
               } else {
                 LOG_INFO("连接服务器成功！");
               }
            }


            virtual void shutdown() override {
               return _client.disconnect();
            }    

            //发送消息
            virtual bool send(const BaseMessage::ptr& msg) override {
               if(connected() == false)
               {
                  LOG_ERROR("连接已断开");
                  return false;
               }
               _conn->send(msg);
               return true;
            }

            //获取连接
            virtual BaseConnection::ptr connection() override {
               return _conn;
            }
            //检查是否已连接
            virtual bool connected() override {
               return _conn != nullptr && _conn->connected() == true;
            }
            
         private:
      //状态判断
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if(conn->connected())
            {
                std::cout << "连接建立！" << std::endl;
                _connect_success = true;

                _conn = ConnectionFactory::create(conn,_protocol);//创建连接对象
                _downlatch.countDown();
            }
            else
            {
                std::cout << "连接关闭！" << std::endl;
                _conn.reset();//重置关闭连接对象
                _connect_success = false;
                _downlatch.countDown(); 
            }
        }
        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp receiveTime)
            {

               auto base_buf = BufferFactory::create<MuduoBuffer>(buf);//创建缓冲区对象
            
               while(1)
               {
                  if(_protocol->canProcessed(base_buf) == false)
                  {
                     //数据不足
                     if(base_buf->readableSize() > _max_data_size)
                     {
                       conn->shutdown();//关闭连接
                       LOG_ERROR("连接关闭缓冲区数据大小超过最大消息大小");
                       break;
                     }
                     break;
                  }
                  //数据足够，解析消息
                  BaseMessage::ptr msg;
                  bool ret = _protocol->onMessage(base_buf, msg);
                  if(ret == false)
                  {
                     conn->shutdown();
                     LOG_ERROR("缓存区数据错误");
                     break;
                  }
                  if(_cb_message)
                  {
                     _cb_message(_conn, msg);
                  }
                  continue;
               }
            }

         private:
         BaseProtocol::ptr _protocol;//协议对象
         BaseConnection::ptr _conn;
         bool _connect_success;
         muduo::CountDownLatch _downlatch;
         muduo::net::EventLoopThread _loopthread;//事件循环线程
         muduo::net::EventLoop *_baseloop;
         muduo::net::TcpClient _client;

         const size_t _max_data_size = (1<<16);//最大消息大小
         std::mutex _mutex;
    };
    class ClientFactory {
    public:
      template <typename... Args>
      static BaseClient::ptr create(Args &&...args) {
        return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
      }
    };
}
