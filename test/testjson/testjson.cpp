#include"json.hpp"
using json = nlohmann::json;

#include<iostream>
#include<vector>
#include<map>
using namespace std;


//json序列化代代码1
void func1()
{
    json js;
    js["msg_type"] = 2;
    js["msg"] = "hello,world";
    js["from"] = "zhang san";
    js["to"] = "li si";

    cout<<js<<endl;
}

//json序列化代码2
void func2()
{
    json js;
    js["id"] = {1,2,3,4,5};
    js["name"] = "zhang san";
    js["msg"]["zhang san"] = "hello";
    js["msg"]["li si"] = "world";

    cout<<js<<endl;
}

int main()
{
    func2();
    return 0;
}