#include "../../common/net.hpp"
#include "../../common/base/uuid.hpp"
#include "../../client/rpc_client.hpp"
#include "source/common/base/logger.hpp"
#include <thread>
void callback(const Json::Value &result) {
    LOG_INFO("callback result: %d", result.asInt());
}

int main()
{
    json_rpc::client::RpcClient client(false, "127.0.0.1", 9090);

    Json::Value params, result;
    params["num1"] = 11;
    params["num2"] = 22;
    bool ret = client.call("Add", params, result);
    if (ret != false) {
        LOG_INFO("result: %d", result.asInt());
    }

    json_rpc::client::RpcCaller::JsonAsyncResponse res_future;
    params["num1"] = 33;
    params["num2"] = 44;
    ret = client.call("Add", params, res_future);
    if (ret != false) {
        result = res_future.get();
        LOG_INFO("result: %d", result.asInt());
    }

    params["num1"] = 55;
    params["num2"] = 66;
    ret = client.call("Add", params, callback);
    LOG_DEBUG("-------\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}