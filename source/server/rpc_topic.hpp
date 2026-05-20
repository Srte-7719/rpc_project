#pragma once
#include "../common/net/net.hpp"
#include "../common/net/message.hpp"
#include <unordered_set>
namespace json_rpc {
    namespace server {
         class TopicManager {
            public:
                using ptr = std::shared_ptr<TopicManager>;
                TopicManager() {}
                void onTopicRequest(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg) {
                    TopicOptype topic_optype = msg->topicOptype();
                    bool ret = true;
                    switch(topic_optype){
                        //主题的创建
                        case TopicOptype::TOPIC_CREATE: topicCreate(conn, msg); break;
                        case TopicOptype::TOPIC_REMOVE: topicRemove(conn, msg); break;
                        case TopicOptype::TOPIC_SUBSCRIBE: ret = topicSubscribe(conn, msg); break;
                        case TopicOptype::TOPIC_CANCEL: topicCancel(conn, msg); break;
                        case TopicOptype::TOPIC_PUBLISH: ret = topicPublish(conn, msg); break;
                        default:  return errorResponse(conn, msg, RCode::RCODE_INVALID_OPTYPE);
                    }
                    if (!ret) return errorResponse(conn, msg, RCode::RCODE_NOT_FOUND_TOPIC);
                    return topicResponse(conn, msg);
                } 
                
                void onShutdown(const BaseConnection::ptr &conn) {
                    std::vector<Topic::ptr> topics;//加入过的主题对象
                    Subscriber::ptr subscriber;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _subscribers.find(conn);
                        if (it == _subscribers.end()) {
                            return;
                        }

                        subscriber = it->second;
                        //获取订阅者关联主题对象
                        for (auto &topic_name : subscriber->topics) {
                            auto topic_it = _topics.find(topic_name);
                            if (topic_it == _topics.end()) continue;
                            topics.push_back(topic_it->second);
                        }
                        _subscribers.erase(it);
                    }
                    //从受影响的主题对象中移除订阅者
                    for (auto &topic : topics) {
                        topic->removeSubscriber(subscriber);
                    }
                }

            private:
            //发送错误响应
            void errorResponse(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg, RCode rcode) {
                auto msg_rsp = MessageFactory::create<TopicResponse>();
                msg_rsp->setId(msg->rid());
                msg_rsp->setMtype(MType::RSP_TOPIC);
                msg_rsp->setRcode(rcode);
                conn->send(msg_rsp);
            }
            //发送主题响应
            void topicResponse(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg) {
                auto msg_rsp = MessageFactory::create<TopicResponse>();
                msg_rsp->setId(msg->rid());
                msg_rsp->setMtype(MType::RSP_TOPIC);
                msg_rsp->setRcode(RCode::RCODE_OK);
                conn->send(msg_rsp);
            }

            //创建主题
            void topicCreate(const BaseConnection::ptr &conn , const TopicRequest::ptr &msg) {
                std::unique_lock<std::mutex> lock(_mutex);
                std::string topic_name = msg->topicKey();//获取主题名称
                auto topic = std::make_shared<Topic>(topic_name);
                _topics.insert(std::make_pair(topic_name, topic));
            }

            //删除主题
           void topicRemove(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg) {
                std::string topic_name = msg->topicKey();//获取主题名称
                std::unordered_set<Subscriber::ptr> subscribers;//存放订阅了这个主题的所有客户端
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _topics.find(topic_name);
                    if (it == _topics.end()) {
                        return ;
                    }
                    subscribers = it->second->subscribers;
                    _topics.erase(it);
                }
            }
            //订阅主题
             bool topicSubscribe(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg) {
                Topic::ptr topic;
                Subscriber::ptr subscriber;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(msg->topicKey());
                    if (topic_it == _topics.end()) {
                        return false;
                    }
                    topic = topic_it->second;
                    auto sub_it = _subscribers.find(conn);
                    if (sub_it != _subscribers.end()) {
                      subscriber = sub_it->second;
                    } else {
                      subscriber = std::make_shared<Subscriber>(conn);
                      _subscribers.insert(std::make_pair(conn, subscriber));
                    }
                }
                topic->appendSubscriber(subscriber);
                subscriber->appendTopic(msg->topicKey());
                return true;
             }

             //取消订阅主题
             void topicCancel(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg) {
                 Topic::ptr topic;
                 Subscriber::ptr subscriber;
                 {
                    std::unique_lock<std::mutex> lock(_mutex);

                    auto topic_it = _topics.find(msg->topicKey());
                    if (topic_it != _topics.end()) {
                        topic =  topic_it->second;
                    }

                    auto sub_it = _subscribers.find(conn);
                    if (sub_it != _subscribers.end()) {
                      subscriber = sub_it->second;
                    }
                 }
                if (subscriber) subscriber->removeTopic(msg->topicKey());
                if (topic && subscriber) topic->removeSubscriber(subscriber);
             }

             //发布主题
             bool topicPublish(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg) {
                 Topic::ptr topic;
                 {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(msg->topicKey());
                    if (topic_it == _topics.end()) {
                        return false;
                    }
                    topic = topic_it->second;
                 }
                 topic->pushMessage(msg);
                 return true;
             }
                
            private:
                //订阅者
                struct Subscriber {
                    using ptr = std::shared_ptr<Subscriber>;
                    std::mutex _mutex;
                    BaseConnection::ptr conn;
                    std::unordered_set<std::string> topics;//订阅者所订阅的主题名称
                    Subscriber(const BaseConnection::ptr &c): conn(c) { }

                    void appendTopic(const std::string &topic_name) {
                        std::unique_lock<std::mutex> lock(_mutex);
                        topics.insert(topic_name);
                    }

                    void removeTopic(const std::string &topic_name) {
                        std::unique_lock<std::mutex> lock(_mutex);
                        topics.erase(topic_name);
                    }
                };
                struct Topic {
                    using ptr = std::shared_ptr<Topic>;
                    std::mutex _mutex;
                    std::string topic_name;
                    std::unordered_set<Subscriber::ptr> subscribers;//主题的订阅者
                    Topic(const std::string &name) : topic_name(name){}
                    //添加订阅者
                    void appendSubscriber(const Subscriber::ptr &subscriber) {
                        std::unique_lock<std::mutex> lock(_mutex);
                        subscribers.insert(subscriber);
                    }
                    //移除订阅者
                    void removeSubscriber(const Subscriber::ptr &subscriber) {
                        std::unique_lock<std::mutex> lock(_mutex);
                        subscribers.erase(subscriber);
                    }
                    void pushMessage(const BaseMessage::ptr &msg) {
                        std::unique_lock<std::mutex> lock(_mutex);
                        for (auto &subscriber : subscribers) {
                            subscriber->conn->send(msg);
                        }
                    }
                };
            private:
                std::mutex _mutex;
                std::unordered_map<std::string, Topic::ptr> _topics;//主题名称到主题的映射
                std::unordered_map<BaseConnection::ptr, Subscriber::ptr> _subscribers;
         };
    }
}