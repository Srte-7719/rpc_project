#include "../../client/rpc_client.hpp"
#include <thread>
void callback(const std::string& key,const std::string& msg)
{
    LOG_INFO("主题 %s 收到消息: %s", key.c_str(), msg.c_str());
}

int main()
{
    //实例化客户端对象
    auto client = std::make_shared<json_rpc::client::TopicClient>("127.0.0.1",7070);
    //创建主题
    bool ret = client->create("hello");
    if(!ret)
    {
       LOG_ERROR("创建主题失败");
    }
    //订阅主题
    ret = client->subscribe("hello",callback);
    if(!ret)
    {
       LOG_ERROR("订阅主题失败");
    }
    //等待退出
    std::this_thread::sleep_for(std::chrono::seconds(10));
    client->shutdown();
    return 0;
}