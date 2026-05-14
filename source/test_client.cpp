#include "message.hpp"
#include "net.hpp"
#include <iostream>
#include <unistd.h>
#include "common/uuid.h"

// 客户端收到服务器响应的回调
void onClientResponse(const json_rpc::BaseConnection::ptr& conn, json_rpc::BaseMessage::ptr& msg)
{
    std::cout << "\n[Client] 收到服务器响应: " << msg->serialize() << std::endl;
    
    // 类型转换，打印响应结果
    auto rpc_rsp = std::dynamic_pointer_cast<json_rpc::RpcResponse>(msg);
    if (rpc_rsp) {
        // ✅ 修复1：枚举类型转成int再输出，或用errReason函数
        std::cout << "[Client] 响应码: " << static_cast<int>(rpc_rsp->rcode()) 
                  << " (" << json_rpc::errReason(rpc_rsp->rcode()) << ")" << std::endl;
        std::cout << "[Client] 响应结果: " << rpc_rsp->result().asInt() << std::endl;
    }
}

int main()
{
    // 1. 创建协议对象
    auto protocol = json_rpc::ProtocolFactory::create<json_rpc::LVProtocol>();
    
    // 2. 创建客户端：连接 127.0.0.1:9090
    auto client = json_rpc::ClientFactory::create<json_rpc::MuduoClient>("127.0.0.1", 9090, protocol);
    
    // 3. 设置响应回调 ✅ 修复3：去掉多余的cb:前缀
    client->setMessageCallback(onClientResponse);
    
    // 4. 连接服务器
    client->connect();
    std::cout << "[Client] 已连接服务器 127.0.0.1:9090" << std::endl;

    // 5. 构造 RPC 请求
    auto rpc_req = json_rpc::MessageFactory::create<json_rpc::RpcRequest>();
    rpc_req->setId(json_rpc::UUID::generate_uuid());  
    rpc_req->setMtype(json_rpc::MType::REQ_RPC);
    rpc_req->setMethod("add");            // 方法名
    rpc_req->setParams(Json::Value(10));  // 参数
    
    // 6. 发送请求
    std::cout << "[Client] 发送请求: " << rpc_req->serialize() << std::endl;
    client->send(rpc_req);

    // 保持程序运行，等待响应
    while (true) {
        sleep(1);
    }

    return 0;
}