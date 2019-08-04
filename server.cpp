/*
服务端类实现
*/
#include "server.h"

using namespace std;

//服务端类构造函数
Server::Server()
{
    //初始化服务器地址和端口
    serverAddr.sin_family = AF_INET;                   //设定IPv4地址协议
    serverAddr.sin_port = htons(SERVER_PORT);          //设定端口,htons()函数用于将主机字节序转换为网络字节序
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP); //设定服务端IP地址

    //初始化监听socket
    listenfd = 0;

    //epollfd
    epfd = 0;
}

//初始化服务端并启动监听
void Server::Init()
{
    cout << "Init Server..." << endl;

    //创建监听socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        perror("listenfd create error");
        exit(-1);
    }

    //绑定地址
    if(bind(listenfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }

    //监听
    int ret = listen(listenfd, 5);
    if(ret < 0)
    {
        perror("listen error");
        exit(-1);
    }

    cout << "Start to listen: " << SERVER_IP << endl;

    //在内核中创建事件表,epfd是一个句柄
    epfd = epoll_create(EPOLL_SIZE);

    if(epfd < 0)
    {
        perror("epfd error");
        exit(-1);
    }

    //往事件表里添加监听事件
    addfd(epfd, listenfd, true);
}

//关闭服务,清理并关闭套接字描述符
void Server::Close()
{
    //关闭socket
    close(listenfd);

    //关闭epoll监听
    close(epfd);
}

//给所有客户端发送广播消息
int Server::SendBroadcastMsg(int clientfd)
{
    //buf[BUF_SIZE] 接受新消息
    //message[BUF_SIZE] 保存格式化消息
    char recv_buf[BUF_SIZE];
    char send_buf[BUF_SIZE];
    Message msg;
    bzero(recv_buf, BUF_SIZE);  //字符数组置为0

    //接受新消息
    cout << "read from client(clientID = " << clientfd << ")" << endl;
    int len = recv(clientfd, recv_buf, BUF_SIZE, 0);

    //清空结构体,把接受到的字符串转换为结构体
    memset(&msg, 0, sizeof(msg));
    memcpy(&msg, recv_buf, sizeof(msg));

    //判断接受到的信息是私聊还是群聊
    msg.srcID = clientfd;
    if (msg.content[0] == '\\' && isdigit(msg.content[1]))
    {
        msg.type = 1;
        msg.destID = msg.content[1] - '0';
        memcpy(msg.content, msg.content + 2, sizeof(msg.content));
    }
    else
        msg.type = 0;

    //如果客户端关闭了连接
    if(len == 0)
    {
        close(clientfd);

        //在客户端列表中删除该客户端
        clients_list.remove(clientfd);
        cout << "ClientID = " << clientfd << " closed.\n"
             << "now there are " << clients_list.size() << " clients in the char room.\n";
    }
    //否则发送消息给所有客户端
    else
    {
        //判断聊天室是否还有其他客户端
        if(clients_list.size() == 1)
        {
            //发送提示消息
            memcpy(msg.content, CAUTION, sizeof(msg.content));
            bzero(send_buf, BUF_SIZE);
            memcpy(send_buf, &msg, sizeof(msg));
            send(clientfd, send_buf, sizeof(send_buf), 0);
            return len;
        }

        //存放格式化后的消息
        char format_message[BUF_SIZE];
        
        //群聊
        if (msg.type == 0)
        {
            //格式化发送的消息内容是#define SERVER_MESSAGE "ClientID %d say >> %s"
            sprintf(format_message, SERVER_MESSAGE, clientfd, msg.content);
            memcpy(msg.content, format_message, BUF_SIZE);

            //遍历客户端列表，依次发送消息，需要判断不要给来源客户端发
            list<int>::iterator itr;
            for (itr = clients_list.begin(); itr != clients_list.end(); ++itr)
            {
                if (*itr != clientfd)
                {
                    //发送的结构体转换为字符串
                    bzero(send_buf, BUF_SIZE);
                    memcpy(send_buf, &msg, sizeof(msg));
                    if (send(*itr, send_buf, sizeof(send_buf), 0) < 0)
                        return -1;
                }
            }
        }
        
        //私聊
        if (msg.type == 1)
        {
            bool private_offline = true;
            sprintf(format_message, SERVER_PRIVATE_MESSAGE, clientfd, msg.content);
            memcpy(msg.content, format_message, BUF_SIZE);

            //遍历客户端列表给指定客户发送消息
            list<int>::iterator itr;
            for (itr = clients_list.begin(); itr != clients_list.end(); ++itr)
            {
                if (*itr == msg.destID)
                {
                    private_offline = false;
                    //把发送的数据结构体转换为字符串
                    bzero(send_buf, BUF_SIZE);
                    memcpy(send_buf, &msg, sizeof(msg));
                    if (send(*itr, send_buf, sizeof(send_buf), 0) < 0)
                        return -1;
                }
            }

            //如果私聊对象不在线
            if (private_offline)
            {
                sprintf(format_message, SERVER_PRIVATE_ERROR_MESSAGE, msg.destID);
                memcpy(msg.content, format_message, BUF_SIZE);
                bzero(send_buf, BUF_SIZE);
                memcpy(send_buf, &msg, sizeof(msg));
                if (send(*itr, send_buf, sizeof(send_buf), 0) < 0)
                    return -1;
            }
        }
    }
    return len;
}

//启动服务器
void Server::Start()
{
    //epoll 事件队列
    static struct epoll_event events[EPOLL_SIZE];

    //初始化服务端
    Init();

    //主循环
    while(1)
    {
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0)
        {
            perror("epoll failure");
            break;
        }

        cout << "epoll_events_count = " << epoll_events_count << endl;

        //处理这些epoll就绪事件
        for (int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //新用户连接
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrLength);

                cout << "client connection from: "
                     << inet_ntoa(client_addr.sin_addr) << ":"
                     << ntohs(client_addr.sin_port) << ", clientfd = "
                     << clientfd << endl;
                
                addfd(epfd, clientfd, true);

                //服务端用list保存用户连接
                clients_list.push_back(clientfd);
                cout << "Add new clientfd = " << clientfd << " to epoll" << endl;
                cout << "Now there are " << clients_list.size() << " clients in the chat room" << endl;

                //服务端发送欢迎信息
                cout << "Welcome message" << endl;
                Message msg;
                char format_msg[BUF_SIZE];
                char send_buf[BUF_SIZE];
                sprintf(format_msg, SERVER_WELCOME, clientfd);
                memcpy(msg.content, format_msg, BUF_SIZE);
                bzero(send_buf, BUF_SIZE);
                memcpy(send_buf, &msg, sizeof(msg));
                int ret = send(clientfd, send_buf, sizeof(send_buf), 0);
                if (ret < 0)
                {
                    perror("send error");
                    Close();
                    exit(-1);
                }
            }
            //处理客户发来的消息并广播
            else
            {
                int ret = SendBroadcastMsg(sockfd);
                if (ret < 0)
                {
                    perror("error");
                    Close();
                    exit(-1);
                }
            }
        }
    }

    //关闭服务
    Close();
}