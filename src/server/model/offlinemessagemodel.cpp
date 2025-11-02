#include "offlinemessagemodel.hpp"

// 存储用户离线信息
void OfflineMsgModel::insert(int usrid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql,"insert into offlinemessage values('%d','%s')",usrid,msg.c_str());

    Mysql mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 删除用户离线信息
void OfflineMsgModel::remove(int usrid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", usrid);

    Mysql mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户离线信息
vector<string> OfflineMsgModel::query(int usrid)
{
     // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", usrid);

    vector<string> vec;
    Mysql mysql;
    if (mysql.connect())
    {
        MYSQL_RES * res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}