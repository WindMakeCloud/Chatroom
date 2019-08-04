/*
客户端类定义
接口定义:
    Connect() 连接服务端
    Close()   退出连接
    Start()   启动客户端
*/
#ifndef CHATROOM_CLIENT_H
#define CHATROOM_CLIENT_H

#include "common.h"

using namespace std;

//客户端类,用来连接服务端发送和接受消息
class Client
{
public:
    //无参数构造函数
    Client();

    //连接服务器
    void Connect();

    //断开连接
    void Close();

    //启动客户端
    void Start();
    
private:
    //当前连接服务器创建的socket
    int sockfd;

    //当前进程ID
    int pid;

    //epoll_create创建后的返回值
    int epfd;

    //创建管道,其中fd[0]用于父进程读,fd[1]用于子进程写
    int pipe_fd[2];

    //表示客户端是否正常工作
    bool isClientWork;

    //聊天信息
    Message msg;

    //结构体要转换为字符串
    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];

    //用户连接的服务器IP和端口
    struct sockaddr_in serverAddr;
};

#endif