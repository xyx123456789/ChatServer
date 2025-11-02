#include "chatServer.hpp"
#include"json.hpp"
#include"chatService.hpp"


#include<functional>
#include<string>
using namespace std;
using namespace placeholders;

ChatServer::ChatServer(EventLoop* loop,
                      const InetAddress& addr,
                      const string & nameOrg)
    : server_(loop,addr,nameOrg),
      loop_(loop)
{
    //注册连接回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));

    //注册信息回调
    server_.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

    //设置线程数量
    server_.setThreadNum(4);

}

void ChatServer::start()
{
    server_.start();
}

//上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    //客户端断开连接
    if(!conn->connected())
    {
      ChatService::Instance()->closeclientException(conn);
      conn->shutdown();
    }

}


//上报读写事件相关信息的回调函数
void ChatServer::ChatServer::onMessage(const TcpConnectionPtr& conn,Buffer * buffer,Timestamp time)
{
  string buf = buffer->retrieveAllAsString();
  
  //数据的反序列化
  json js = json::parse(buf);
  //达到的目的，完全解耦网络模块的代码和业务模块的代码
  //通过js["msgid"] -> Handler -> conn js time
  auto msgHanlder = ChatService::Instance()->getHanlder(js["msgid"].get<int>());
  //回调消息绑定好的事件处理器，来执行相应的业务操作
  msgHanlder(conn,js,time);
}