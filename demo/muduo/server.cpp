#include "muduo/net/Callbacks.h"
#include <muduo/net/EventLoop.h> // 事件循环
#include <muduo/net//TcpConnection.h> // TCP连接封装
#include <muduo/net/Buffer.h>// 网络缓冲区
#include <muduo/net/TcpServer.h>// TCP服务器
#include <iostream>
#include <string>
#include <unordered_map>

class DictServer{
    public:
        DictServer(int port)
        : _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),"DictServer",muduo::net::TcpServer::kNoReusePort)
        {
            
            _server.setConnectionCallback(std::bind(&DictServer::onConnection, this, std::placeholders::_1));// 设置连接回调函数
            _server.setMessageCallback(std::bind(&DictServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));// 设置消息回调函数
        }

        void start()
        {
            _server.start();//开始监听
            _baseloop.loop();// 进入事件循环
        }
        
    private:
    //状态判断
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if(conn->connected())
            {
                std::cout << "连接建立！" << std::endl;
            }
            else
            {
                std::cout << "连接关闭！" << std::endl;
            }
        }
        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp receiveTime)
            {
                static std::unordered_map<std::string, std::string> dict={
                    {"hello", "你好"},
                    {"how are you", "我很好"},
                    {"goodbye", "再见"},
                    {"bye", "再见"},
                };  
                std::string msg = buf->retrieveAllAsString();// 从缓冲区中读取所有数据
                std::string response;// 响应消息
                auto it = dict.find(msg);
                if(it != dict.end())
                {
                    response = it->second;
                }
                else
                {
                    response = "未知单词";
                }
                conn->send(response);
            }
    private:
        muduo::net::EventLoop _baseloop;
        muduo::net::TcpServer _server;
};

int main()
{
    DictServer server(9090);
    server.start();
    return 0;
}