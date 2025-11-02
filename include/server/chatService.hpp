#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<muduo/net/TcpConnection.h>
#include<functional>
#include<unordered_map>
#include<mutex>
#include<vector>

#include"json.hpp"
#include"usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"


using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

//处理消息事件回调方法的类型
using MsgHanlder = std::function<void(const TcpConnectionPtr& conn,json& js,Timestamp time)>;

class ChatService{
public:
    static ChatService *Instance();

    //处理登录业务
    void login(const TcpConnectionPtr & conn,json & js,Timestamp time);

    //处理注册业务
    void reg(const TcpConnectionPtr & conn,json & js,Timestamp time);

    //一对一聊天业务
    void oneChat(const TcpConnectionPtr & conn,json & js,Timestamp time);

    //添加好友业务
    void addFriend(const TcpConnectionPtr & conn,json & js,Timestamp time);

     // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

     // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //获取消息对应的处理器
    MsgHanlder getHanlder(int msgid);

    //处理客户端异常退出
    void closeclientException(const TcpConnectionPtr& conn);

    //服务器异常，处理用户状态，业务重置方法
    void reset();

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    ChatService();

    //存储消息id和其对应的处理方法
    unordered_map<int,MsgHanlder> msgHanlderMap_;

    //存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> userConnMap_;

    //定义互斥锁，保证userConnMap_线程安全
    mutex connMutex_;

    //数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    //Redis操作对象
    Redis redis_;

};



#endif