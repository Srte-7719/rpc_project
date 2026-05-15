#include "message.hpp"
#include "net.hpp"


void onMessage(const json_rpc::BaseConnection::ptr& conn, json_rpc::BaseMessage::ptr& msg)
{
    std::string body = msg->serialize();
    std::cout << "收到消息: " << body << std::endl;
    auto rpc_rsp = json_rpc::MessageFactory::create<json_rpc::RpcResponse>();
    rpc_rsp->setId(msg->rid());
    rpc_rsp->setMtype(json_rpc::MType::RSP_RPC);
    rpc_rsp->setRcode(json_rpc::RCode::RCODE_OK);
    rpc_rsp->setResult(Json::Value(33));
    conn->send(rpc_rsp);
}

int main()
{
    auto protocol = json_rpc::ProtocolFactory::create<json_rpc::LVProtocol>();
    auto server = json_rpc::ServerFactory::create<json_rpc::MuduoServer>(9090, protocol);
    
    server->setMessageCallback(onMessage);
    server->start();
    return 0;
}