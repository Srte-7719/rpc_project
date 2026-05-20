#include "../../server/rpc_server.hpp"

void Add(const Json::Value &req, Json::Value &rsp) {
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}
int main()
{
    std::unique_ptr<json_rpc::server::SDescribeFactory> desc_factory(new json_rpc::server::SDescribeFactory());
    desc_factory->setMethodName("Add");
    desc_factory->setParamsDescribe("num1", json_rpc::server::VType::INTEGRAL);
    desc_factory->setParamsDescribe("num2", json_rpc::server::VType::INTEGRAL);
    desc_factory->setReturnType(json_rpc::server::VType::INTEGRAL);
    desc_factory->setCallback(Add);

    json_rpc::server::RpcServer server(json_rpc::Address("127.0.0.1", 9090),
    true, 
    json_rpc::Address("127.0.0.1", 8080) 
    );
    server.registerMethod(desc_factory->build());
    server.start();
    return 0;
}