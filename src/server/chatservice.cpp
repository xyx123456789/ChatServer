#include "chatService.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

ChatService *ChatService::Instance()
{
    static ChatService service;
    return &service;
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = userModel_.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        // 该用户已经登录，不允许重复登录
        if (user.getState() == "online")
        {
            json res;
            res["msgid"] = LOGIN_MSG_ACK;
            res["errno"] = 2;
            res["errmsg"] = "该账号已经登录";
            conn->send(res.dump());
        }
        else
        {
            // 登录成功，记录信息
            {
                lock_guard<mutex> lock(connMutex_);
                userConnMap_.insert({id, conn});
            }

            //id用户登录成功后，向redis订阅channer(id)
            redis_.subscribe(id);

            // 登录成功，更新信息
            user.setState("online");
            userModel_.updateState(user);

            json res;
            res["msgid"] = LOGIN_MSG_ACK;
            res["errno"] = 0;
            res["id"] = user.getId();
            res["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = offlineMsgModel_.query(id);
            if (!vec.empty())
            {
                res["offlinemsg"] = vec;
                offlineMsgModel_.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = friendModel_.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                res["friends"] = vec2;
            }

            //查询用户的群组信息
            vector<Group> groupuserVec = groupModel_.queryGroups(id);
            if(!groupuserVec.empty())
            {
                vector<string> groupV;
                for(Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                res["groups"] = groupV;
            }

            conn->send(res.dump());
        }
    }
    else
    {
        // 该用户不存在
        json res;
        res["msgid"] = LOGIN_MSG_ACK;
        res["errno"] = 1;
        res["errmsg"] = "该用户不存在或者账号密码错误";
        conn->send(res.dump());
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    bool st = userModel_.insert(user);
    if (st)
    {
        // 注册成功
        json res;
        res["msgid"] = REG_MSG_ACK;
        res["errno"] = 0;
        res["id"] = user.getId();
        conn->send(res.dump());
    }
    else
    {
        // 注册失败
        json res;
        res["msgid"] = REG_MSG_ACK;
        res["errno"] = 1;
        conn->send(res.dump());
    }
}

// 注册消息以及对应的Hanlder的回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    msgHanlderMap_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHanlderMap_.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});
    msgHanlderMap_.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    msgHanlderMap_.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    msgHanlderMap_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    msgHanlderMap_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHanlderMap_.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    msgHanlderMap_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    //连接redis服务器
    if(redis_.connect())
    {
        //设置上报消息的回调
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }


}

// 获取消息对应的处理器
MsgHanlder ChatService::getHanlder(int msgid)
{
    auto it = msgHanlderMap_.find(msgid);
    if (it == msgHanlderMap_.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid " << msgid << "is not exist";
        };
    }
    else
    {
        return msgHanlderMap_[msgid];
    }
}

// 处理客户端异常退出
void ChatService::closeclientException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }

    //用户异常退出后，取消订阅通道
    redis_.unsubscribe(user.getId());

    if (user.getId() != -1)
    {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end())
        {
            // toid在线，转发消息   服务器主动推送消息到toid
            it->second->send(js.dump());
            return;
        }
    }

    //查询toid是否在线,没在一台电脑上登录
    User user = userModel_.query(toid);
    if(user.getState() == "online")
    {
        //toid在线，向redis订阅通道发送消息
        redis_.publish(toid,js.dump());
        return;
    }

    // toid不在线，存储离线消息
    offlineMsgModel_.insert(toid, js.dump());
}

// 服务器异常，处理用户状态，业务重置方法
void ChatService::reset()
{
    // 把online用户设置成offline
    userModel_.resetState();
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 添加好友信息
    friendModel_.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1,name,desc);
    if(groupModel_.createGroup(group))
    {
        //存储群组创建人信息
        groupModel_.addGroup(userid,group.getId(),"creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    
    vector<int> useridVec = groupModel_.queryGroupUsers(userid,groupid);

    lock_guard<mutex> lock(connMutex_);
    for(auto id : useridVec)
    {
        auto it = userConnMap_.find(id);
        if(it != userConnMap_.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            //查询toid是否在线
            User user = userModel_.query(id);
            if(user.getState() == "online")
            {
                //toid在线，向redis订阅通道发送消息
                redis_.publish(id,js.dump());
            }
            else
            {
                //toid不在线，存储离线消息
                offlineMsgModel_.insert(id,js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(userid);
        if(it != userConnMap_.end())
        {
            userConnMap_.erase(it);
        }
    }

    //用户注销后，取消订阅通道
    redis_.unsubscribe(userid);

    User user(userid, "", "", "offline");
    userModel_.updateState(user);
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(connMutex_);
    auto it = userConnMap_.find(userid);
    if(it != userConnMap_.end())
    {
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    offlineMsgModel_.insert(userid,msg);
}