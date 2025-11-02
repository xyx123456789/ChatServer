#include<mysql/mysql.h>
#include<string>
using namespace std;

/*
实现MySQL数据库的操作
*/
class Mysql
{
public:
	// 初始化数据库连接
	Mysql();
	// 释放数据库连接资源
	~Mysql();
	// 连接数据库
    bool connect();
	// 更新操作 insert、delete、update
	bool update(string sql);
	// 查询操作 select
	MYSQL_RES *query(string sql);
	//获取连接
	MYSQL* getConnection();

private:
	MYSQL *_conn;		// 表示和MySQL Server的一条连接
};


