#pragma once
#include "requestor.hpp"
#include "source/common/fields.hpp"
#include <unordered_set>

namespace json_rpc {
    namespace client {
         class TopicManager {
            public:
                using SubCallback = std::function<void(const std::string &key, const std::string &msg)>;
                using ptr = std::shared_ptr<TopicManager>;
                TopicManager(const Requestor::ptr &requestor) : _requestor(requestor) {}
                bool create(const BaseConnection::ptr &conn, const std::string &key) {
                   return commonRequest(conn, key, TopicOptype::TOPIC_CREATE);
                }
                bool remove(const BaseConnection::ptr &conn, const std::string &key) {
                    return commonRequest(conn, key, TopicOptype::TOPIC_REMOVE);
                }
                bool subscribe(const BaseConnection::ptr &conn, const std::string &key, const SubCallback &cb) {
                    addSubscribe(key, cb);
                    bool ret = commonRequest(conn, key, TopicOptype::TOPIC_SUBSCRIBE);
                    if (ret == false) {
                        delSubscribe(key);
                        return false;
                    }
                    return true;
                }
                bool cancel(const BaseConnection::ptr &conn, const std::string &key) {
                    delSubscribe(key);
                    return commonRequest(conn, key, TopicOptype::TOPIC_CANCEL);
                }
                bool publish(const BaseConnection::ptr &conn, const std::string &key, const std::string &msg) {
                    return commonRequest(conn, key, TopicOptype::TOPIC_PUBLISH, msg);
                }
                //提供dispatcher调用
                void onPublish(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg) {
                    //1. 从消息中取出操作类型进行判断，是否是消息请求
                    auto type = msg->topicOptype();
                    if (type != TopicOptype::TOPIC_PUBLISH) {
                        LOG_ERROR("收到了错误类型的主题操作！");
                        return ;
                    }
                    //2. 取出消息主题名称，以及消息内容
                    std::string topic_key = msg->topicKey();
                    std::string topic_msg = msg->topicMsg();
                    //3. 通过主题名称，查找对应主题的回调处理函数，有在处理，无在报错
                    auto callback = getSubscribe(topic_key);
                    if (!callback) {
                        LOG_ERROR("收到了 %s 主题消息，但是该消息无主题处理回调！", topic_key.c_str());
                        return ;
                    }
                    return callback(topic_key, topic_msg);
                }
            private:
                void addSubscribe(const std::string &key, const SubCallback &cb) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _topic_callbacks.insert(std::make_pair(key, cb));
                }
                void delSubscribe(const std::string &key) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _topic_callbacks.erase(key);
                }
                const SubCallback getSubscribe(const std::string &key) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _topic_callbacks.find(key);
                    if (it == _topic_callbacks.end()) {
                        return nullptr;
                    }
                    return it->second;
                 }
                bool commonRequest(const BaseConnection::ptr &conn, const std::string &key, TopicOptype type, const std::string &msg = "") {
                     //构造请求对象填充数据
                    auto msg_req = std::make_shared<TopicRequest>();
                    msg_req->setId(UUID::generate_uuid());
                    msg_req->setMtype(MType::REQ_TOPIC);
                    msg_req->setTopicOptype(TopicOptype::TOPIC_CREATE);
                    msg_req->setTopicKey(key);
                    //发送等待服务器相应
                    BaseMessage::ptr msg_rsp;
                    bool ret = _requestor->send(conn, msg_req, msg_rsp);
                    if (ret == false) {
                        LOG_ERROR("主题操作请求失败");
                        return false;
                    }
                    //检验
                     auto topic_rsp_msg = std::dynamic_pointer_cast<TopicResponse>(msg_rsp);
                    if (!topic_rsp_msg) {
                        LOG_ERROR("主题操作响应，向下类型转换失败！");
                        return false;
                    }
                    if (topic_rsp_msg->rcode() != RCode::RCODE_OK) {
                        LOG_ERROR("主题操作请求出错：%s",errReason(topic_rsp_msg->rcode()).c_str());
                        return false;
                    }
                     return true;
                }
            private:
                std::mutex _mutex;
                std::unordered_map<std::string, SubCallback> _topic_callbacks;//key -> callback
                Requestor::ptr _requestor;//请求器
         };
    }
}