#include "../../server/rpc_server.hpp"

int main() 
{
    json_rpc::server::RegistryServer reg_server(8080);
    reg_server.start();
    return 0;
}