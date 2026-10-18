#include<Acceptor.h>
#include<InetAddress.h>

#include<unistd.h>

namespace KeeServer{

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr)
    :loop_(loop),
    acceptSocket_(Socket::createNonblocking()),//fd给socket实例
    acceptChannel_(loop,acceptSocket_.fd()),//fd给channel
    listenning_(false){
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback([this]{handleRead();});//方便event触发读回调
}
//析构函数
Acceptor::~Acceptor(){
    //acceptChannel_.disableAll();
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


void Acceptor::listen(){
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();//开放读权限
}

void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(newConnectionCallback_){
            newConnectionCallback_(connfd,peerAddr);
        }else{
            ::close(connfd);
        }
    }
}
}