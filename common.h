/*
common.h
1.定义共用的宏定义
2.定义共用的网络编程头文件
3.定义一个函数将文件描述符fd添加到epfd表示的内核事件列表中供客户端和服务端两个类使用
4.定义一个消息数据结构，表示传送的消息，结构体包括发送方fd，接收方fd，用于表示消息类别的type以及文字信息

因为函数recv(),send(),write(),read()参数传递的是字符串，因此在传送前把结构体转换为字符串，接收后把字符串转换为结构体
*/
#ifndef CHATROOM_COMMON_H
#define CHATROOM_COMMON_H

#include <iostream>
#include <string>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//默认服务器端IP地址
#define SERVER_IP "127.0.0.1"

//服务器端口号
#define SERVER_PORT 8888

//epoll支持的最大句柄数
//int epoll_create(int size); 中的size
#define EPOLL_SIZE 5000

//缓冲区大小65535
#define BUF_SIZE 0xFFFF

//新用户登陆后的欢迎消息
#define SERVER_WELCOME "Welcome to join the chat room. Your char ID is: Client #%d"

//其他用户收到消息的前缀
#define SERVER_MESSAGE "ClientID %d say >> %s"
#define SERVER_PRIVATE_MESSAGE "Client %d say to you privately >> %s"
#define SERVER_PRIVATE_ERROR_MESSAGE "Client %d is not in the chat room yet~"

//退出系统
#define EXIT "EXIT"

//提醒你是聊天室中的唯一客户
#define CAUTION "There is only one in the chat room!"

//注册新的fd到epollfd中
//参数enable_et表示是否启用ET模式，如果为True表示启用，否则使用LT模式
static void addfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(enable_et)
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);  //事件注册函数

    //设置socket为非阻塞模式
    //套接字立刻返回，不管I/O是否返回，该函数所在的线程会继续运行
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}

//定义信息结构体，在服务端和客户端之间传送的消息
struct Message
{
    int type;                 //消息类型，定义群消息还是私人消息
    int srcID;                //源ID
    int destID;               //目的ID
    char content[BUF_SIZE];   //消息内容
};

#endif
