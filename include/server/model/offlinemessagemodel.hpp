
#include"db.h"
#include<string>
#include<vector>
using namespace std;

class OfflineMsgModel{
public:
    //存储用户离线信息
    void insert(int usrid,string msg);

    //删除用户离线信息
    void remove(int usrid);

    //查询用户离线信息
    vector<string> query(int usrid);
};

