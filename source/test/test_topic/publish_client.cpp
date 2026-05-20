#include "../../client/rpc_client.hpp"

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
    //发布消息
    for (int i = 0; i < 10; i++)
    {
        ret = client->publish("hello", "hello world " + std::to_string(i));
        if (!ret) {
            LOG_ERROR("发布消息失败");
        }
    }
    client->shutdown();

}