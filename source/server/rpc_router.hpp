#pragma once
#include "../common/net.hpp"
#include "../common/message.hpp"
#include <json/json.h>
namespace  json_rpc {
    namespace server {
        enum class VType {
                BOOL = 0,//布尔值
                INTEGRAL,//整数
                NUMERIC,//数值
                STRING,//字符串
                ARRAY,//数组
                OBJECT,//对象
            };

        class ServiceDescribe{
            public:
                using ptr = std::shared_ptr<ServiceDescribe>;
                using ServiceCallback = std::function<void(const Json::Value&,Json::Value&)>;
                using ParamsDescribe = std::pair<std::string,VType>;

                //构造函数
                ServiceDescribe(std::string &&name,std::vector<ParamsDescribe> &&desc, VType vtype,ServiceCallback &&handler)
                :_method_name(std::move(name)),
                _params_describe(std::move(desc)),
                _return_type(vtype),
                _callback(std::move(handler))
                {}
                const std::string& method() const { return _method_name; }
                //执行业务回调，并检查返回值类型
                 bool paramCheck(const Json::Value &params) const{
                    for (auto &desc : _params_describe) {
                        if (!params.isMember(desc.first)) {
                            LOG_ERROR("参数字段完整性校验失败！%s字段缺失", desc.first.c_str());
                            return false;
                        }
                        
                        if (!check(desc.second, params[desc.first])) {
                            LOG_ERROR("%s参数类型校验失败", desc.first.c_str());
                            return false;
                        }
                    }
                }

                bool call(const Json::Value &params, Json::Value &result) {
                     _callback(params, result);
                     return rtypeCheck(result);
                }



            private:

                bool rtypeCheck(const Json::Value &val) const 
                {
                    return check(_return_type, val);
                }
                bool check(VType vtype, const Json::Value &val) const {
                  switch (vtype) {
                  case VType::BOOL:
                    return val.isBool();
                  case VType::INTEGRAL:
                    return val.isIntegral();
                  case VType::NUMERIC:
                    return val.isNumeric();
                  case VType::STRING:
                    return val.isString();
                  case VType::ARRAY:
                    return val.isArray();
                  case VType::OBJECT:
                    return val.isObject();
                  }
                  return false;
                }
                std::vector<ParamsDescribe> _params_describe;//逐个遍历+参数描述
                std::string _method_name;//方法名
                ServiceCallback _callback;//回调函数
                VType _return_type;//返回值类型
        };


        //建造者模式
        //用于创建ServiceDescribe对象
        class SDescribeFactory {
            public:
                void setMethodName(const std::string &name) { _method_name = name; }
                void setReturnType(VType vtype) { _return_type = vtype; }
                void setParamsDescribe(const std::vector<ServiceDescribe::ParamsDescribe> &desc) { _params_describe = desc; }
                void setCallback(ServiceDescribe::ServiceCallback cb) { _callback = cb; }
                ServiceDescribe::ptr build() {
                    return std::make_shared<ServiceDescribe>(std::move(_method_name), std::move(_params_describe), _return_type, std::move(_callback));
                }
            private:
                std::string _method_name;//方法名
                VType _return_type;//返回值类型
                std::vector<ServiceDescribe::ParamsDescribe> _params_describe;//参数描述
                ServiceDescribe::ServiceCallback _callback;//回调函数
        
        };
        class ServiceManager{
            public:
                using ptr = std::shared_ptr<ServiceManager>;
               void insert(const ServiceDescribe::ptr &desc) {  
                    std::lock_guard<std::mutex> lock(_mutex_);
                    _services[desc->method()] = desc;
                }
                ServiceDescribe::ptr select(const std::string &method_name) {
                    std::lock_guard<std::mutex> lock(_mutex_);
                    auto it = _services.find(method_name);
                    if (it == _services.end()) return ServiceDescribe::ptr();
                    return it->second;
                }
                void remove(const std::string &method_name) {
                    std::lock_guard<std::mutex> lock(_mutex_);
                    _services.erase(method_name);
                }

            private:
                std::mutex _mutex_;
                std::unordered_map<std::string, ServiceDescribe::ptr> _services;
        };

        class RpcRouter{
            public:
                 using ptr = std::shared_ptr<RpcRouter>;
                RpcRouter() : _service_manager(std::make_shared<ServiceManager>()) {}
            //这是注册到Dispatcher的回调函数，用于处理RpcRequest
            //根据方法名调用对应的ServiceDescribe对象
                void onRpcRequest(const BaseConnection::ptr &conn, RpcRequest::ptr &request)
                {
                    auto service = _service_manager->select(request->getMethod());
                    if (!service) {
                        LOG_ERROR("%s 服务未找到！", request->getMethod().c_str());
                       return response(conn, request, Json::Value(), RCode::RCODE_NOT_FOUND_SERVICE);
                    }
                    if (!service->paramCheck(request->getParams())) {
                        LOG_ERROR("%s 服务参数校验失败！", request->getMethod().c_str());
                        return response(conn, request, Json::Value(), RCode::RCODE_INVALID_PARAMS);
                    }
                    Json::Value result;
                    if (!service->call(request->getParams(), result)) {
                        LOG_ERROR("%s 服务调用失败！", request->getMethod().c_str());
                        return response(conn, request, Json::Value(), RCode::RCODE_INTERNAL_ERROR);
                    }
                    response(conn, request, result, RCode::RCODE_OK);
                }
                void registerMethod(const ServiceDescribe::ptr &service) {
                    _service_manager->insert(service);
                }

            private:
              void response(const BaseConnection::ptr &conn,
                            const RpcRequest::ptr &req, const Json::Value &res,
                            RCode rcode) {
                auto msg = MessageFactory::create<RpcResponse>();
                msg->setId(req->rid()); // 请求ID原样带回，客户端靠这个匹配请求和响应
                msg->setMtype(MType::RSP_RPC);
                msg->setRcode(rcode);
                msg->setResult(res);
                conn->send(msg);
              }
            
              ServiceManager::ptr _service_manager;
                

        };
    
    }
}
