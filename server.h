/*
服务端类定义
接口定义:
    Init() 初始化
    Start() 启动服务
    Close() 关闭服务
    SendBroadcastMsg() 广播消息给所有客户端

服务端的主循环先检查并处理EPOLL中的就绪事件，就绪事件主要有两种类型:新连接或者新消息。
服务端会依次从就绪时间表中提取事件并进行处理，如果是新连接则accept()然后addfd()，如果是
新消息则SendBroadcastMsg()。
*/
#ifndef CHATROOM_SERVER_H
#define CHATROOM_SERVER_H

#include "common.h"

using namespace std;

//服务端类，处理客户请求
class Server
{
public:
    //无参数构造函数
    Server();

    //初始化服务端设置
    void Init();

    //关闭服务端
    void Close();

    //启动服务端
    void Start();

private:
    //给所有客户端广播消息
    int SendBroadcastMsg(int clientfd);

    //服务端serverAddr信息
    struct sockaddr_in serverAddr;

    //创建监听socket
    int listenfd;

    //epoll_create创建后的返回值
    int epfd;

    //客户端列表
    list<int> clients_list;
};

#endif