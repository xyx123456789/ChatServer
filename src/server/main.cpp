#include "chatServer.hpp"
#include "chatService.hpp"

#include<signal.h>
#include<iostream>
using namespace std;

//处理服务器ctrl+c退出后，重置用户的状态信息
void restHandler(int)
{
    ChatService::Instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{

    signal(SIGINT,restHandler);

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}