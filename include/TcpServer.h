#pragma once

#include<Acceptor.h>
#include<EventLoopThreadPool.h>
#include<InetAddress.h>
#include<TcpConnection.h>

#include<functional>
#include<memory>
#include<string>
#include<unordered_map>

namespace KeeServer{

class EventLoop;

class TcpServer{
public:
    //回调函数
    using MessageCallback = TcpConnection::MessageCallback;
    using ConnectionCallback = TcpConnection::ConnectionCallback;
    using WriteCompleteCallback = TcpConnection::WriteCompleteCallback;
    using ThreadInitCallback = EventLoopThreadPool::ThreadInitialCallback;
    using ConnectionMap = std::unordered_map<std::string,std::unique_ptr<TcpConnection>>;
    //构造函数和析构函数
    TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& nameArg);
    ~TcpServer() = default;
    //禁用拷贝构造和赋值构造
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    //登记业务，设置回调函数
    void setMessageCallback(const MessageCallback& cb){messageCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_ = cb;}
    void setThreadInitCallback(const ThreadInitCallback& cb){threadInitCallback_ = cb;}
    void setThreadNum(int numThreads){threadPool_.setThreadNum(numThreads);}
    
    void start();//真正营业

private:
    void newConnection(int sockfd,const InetAddress& peerAddr);
    void removeConnection(TcpConnection* conn);

    EventLoop* loop_;
    const std::string name_;
    const std::string ipPort_;
    Acceptor acceptor_;//listen fd
    EventLoopThreadPool threadPool_;//线程池
    MessageCallback messageCallback_;
    ConnectionCallback connectionCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    int nextConnId_;//连接序号
    bool started_;//是否已经start()/listen
    ConnectionMap connections_;//当前所有连接
};
}