#include<TcpServer.h>
#include<EventLoop.h>

#include<cstdio>
#include<cstring>
#include<sys/socket.h>

namespace KeeServer{

TcpServer::TcpServer(EventLoop* loop,
                    const InetAddress& listenAddr,
                    const std::string& nameArg)
    :loop_(loop)
    ,name_(nameArg)
    ,ipPort_(listenAddr.toIpPort())
    ,acceptor_(loop,listenAddr)//listen fd
    ,threadPool_(loop,nameArg)
    ,nextConnId_(1)
    ,started_(false){
        acceptor_.setNewConnectionCallback([this](int sockfd,const InetAddress& peerAddr){
            newConnection(sockfd,peerAddr);
        });
    }

void TcpServer::start(){//开始监听
    if(started_){
        return ;
    }
    started_ = true;
    threadPool_.start(threadInitCallback_);
    acceptor_.listen();
}

void TcpServer::newConnection(int sockfd,const InetAddress& peerAddr){//对端地址
    EventLoop* ioLoop = threadPool_.getNextLoop();//轮询获取一个io线程

    char buf[64] = {};
    //新连接取一个不重复的名字
    std::snprintf(buf,sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    const std::string connName = name_ + buf;

    sockaddr_in local{};
    socklen_t addrlen = sizeof(local);
    ::getsockname(sockfd,reinterpret_cast<sockaddr*>(&local),&addrlen);//对端地址

    auto conn = std::make_unique<TcpConnection>(
        ioLoop,connName,sockfd,InetAddress(local),peerAddr);
    TcpConnection* raw = conn.get();
    connections_[connName] = std::move(conn);

    raw->setMessageCallback(messageCallback_);
    raw->setConnectionCallback(connectionCallback_);
    raw->setWriteCompleteCallback(writeCompleteCallback_);
    raw->setCloseCallback([this](TcpConnection* c){removeConnection(c);});
    ioLoop->runInLoop([raw]{raw->connectEstablished();});//在io线程中调用connectEstablished
}

void TcpServer::removeConnection(TcpConnection* conn){
    const std::string& name = conn->name();
    loop_->runInLoop([this,name]{
        connections_.erase(name);
    });
}
}