#pragma once

#include<Buffer.h>
#include<Channel.h>
#include<InetAddress.h>
#include<Socket.h>

#include<functional>
#include<string>
#include<sys/types.h>

namespace KeeServer{

class EventLoop;

class TcpConnection{

public:
    using MessageCallback = std::function<void(TcpConnection*,Buffer*)>;//回调函数
    using ConnectionCallback = std::function<void(TcpConnection*)>;
    using CloseCallback = std::function<void(TcpConnection*)>;
    using WriteCompleteCallback = std::function<void(TcpConnection*)>;
    using HighWaterMarkCallback = std::function<void(TcpConnection*,size_t)>;

    TcpConnection(EventLoop* loop,
                const std::string& nameArg,
                int sockfd,//connfd
                const InetAddress& localAddr,//本机地址
                const InetAddress& peerAddr//对端地址
    );
    ~TcpConnection();

    TcpConnection(const TcpConnection&)=delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    void connectEstablished();//连接正式可用
    void send(const std::string& buf);//对端写数据
    void sendFile(int fileDescriptor,off_t offset,size_t count);
    void shutdown();

    void setMessageCallback(const MessageCallback& cb){messageCallback_ = cb;}//登记回调
    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_ = cb;}
    void setCloseCallback(const CloseCallback& cb){closeCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_ = cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,size_t highWaterMark){
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    
    const std::string& name()const{return name_;}
    const InetAddress& peerAddress()const{return peerAddr_;}//返回对端地址
    bool connected()const{return state_ == kConnected;}//测试确认连接建立成功
    int fd()const{return channel_.fd();}//返回connfd
private:
    enum StateE{kDisconnected,kConnecting,kConnected,kDisconnecting};

    void sendInLoop(const char* data, size_t len);//在io线程中发送数据
    void sendFileInLoop(int fileDescriptor,off_t offset,size_t count);
    void shutdownInLoop();
    void handleWrite();//处理可写事件
    void handleRead();//处理可读事件
    void handleClose();//处理断开
    void handleError();

    EventLoop* loop_;
    const std::string name_;//连接名字，常量
    StateE state_;//连接状态
    Socket socket_;
    Channel channel_;
    const InetAddress localAddr_;//本机ip host
    const InetAddress peerAddr_;//对端ip host
    MessageCallback messageCallback_;
    ConnectionCallback connectionCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;
    Buffer inputBuffer_;//用户态接收缓冲区
    Buffer outputBuffer_;//用户态发送缓冲区
};
}
