#pragma once

#include<Channel.h>
#include<Socket.h>

#include<functional>

namespace KeeServer{

class EventLoop;//事件循环
class InetAddress;//ip host存储以及翻译
//组合Socket 和Channel
class Acceptor{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;//对端connfd 以及对端地址

    //构造函数和析构函数
    Acceptor(EventLoop* loop, const InetAddress& listenAddr);
    ~Acceptor();

    //禁止拷贝构造和赋值
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    //设置回调函数
    void setNewConnectionCallback(const NewConnectionCallback& cb){
        newConnectionCallback_ = cb;
    }
    //是都在监听
    bool listenning()const{return listenning_;}

    //真正营业
    void listen();//营业+门口登记epoll

private:
    void handleRead();//队列存在连接，从accept中取出fd，

    EventLoop* loop_;//值班台
    Socket acceptSocket_;//listen 套接字
    Channel acceptChannel_;//fd的登记卡
    NewConnectionCallback newConnectionCallback_;//回调函数
    bool listenning_;//有无已经listen
};
}