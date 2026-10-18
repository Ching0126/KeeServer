#include<Acceptor.h>
#include<EventLoop.h>
#include<InetAddress.h>
#include<TcpConnection.h>

#include<arpa/inet.h>
#include<chrono>
#include<fcntl.h>
#include<future>
#include<iostream>
#include<memory>
#include<sys/socket.h>
#include<thread>
#include<unistd.h>

namespace{

int connectTo(uint16_t port){
    int fd = ::socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        return -1;
    }
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port= ::htons(port);
    ::inet_pton(AF_INET,"127.0.0.1",&dest.sin_addr);
    if(::connect(fd,reinterpret_cast<sockaddr*>(&dest),sizeof(dest))<0){
        ::close(fd);
        return -1;
    }
    return fd;
}
}

int main(){
    const uint16_t port = 18766;
    KeeServer::EventLoop loop;//epoll实例
    KeeServer::Acceptor acceptor(&loop,KeeServer::InetAddress(port,"127.0.0.1"));
    std::unique_ptr<KeeServer::TcpConnection> conn;//独立指针

    acceptor.setNewConnectionCallback([&](int fd,const KeeServer::InetAddress& peer){
    conn = std::make_unique<KeeServer::TcpConnection>(&loop,"conn1",fd,KeeServer::InetAddress(),peer);//创建tcpconnection实例，并且独立指针
    conn->setMessageCallback([&](KeeServer::TcpConnection* c,KeeServer::Buffer* buf){
        const std::string msg = buf->retrieveAllAsString();//获取缓冲区所有字母，取出。
        c->send(msg);//发送
        loop.quit();
    });
    conn->connectEstablished();//改变状态，以及登记卡记录。
    });
    acceptor.listen();//想听可读

    int client = connectTo(port);
    if(client<0){
        std::cerr<<"FAIL:connect\n";
        return 1;
    }
    if(::write(client,"hello",5)!=5){
        std::cerr<<"FAIL:client write\n";
        return 1;
    }

    std::promise<void> done;
    std::thread t([&]{
        loop.loop();
        done.set_value();
    });
    /*
        内核：就绪列表里有 listen fd，连同当初塞的 Channel* 还你
        loop：找回 acceptChannel_ → handleEvent → 读回调
            → Acceptor::handleRead
            → accept() 从队列领人，得到 connfd
            → 这时才执行外层回调：
                造 TcpConnection（接管 connfd）
                登记内层消息回调（回声）
                connectEstablished()
                    state = 已连接
                    channel_.enableReading()
                    epoll_ctl(ADD, connfd, EPOLLIN)   ← 内线也进兴趣列表
    */
    auto fut = done.get_future();
    if(fut.wait_for(std::chrono::seconds(1))!= std::future_status::ready){
        std::cerr<<"FAIL:connfd readable did not reach handleRead\n";
        loop.quit();
        std::abort();
    }
    t.join();

    if(!conn ||!conn->connected()){
        std::cerr<<"FAIL:connection should be established\n";
        ::close(client);
        return 1;
    }

    ::fcntl(client,F_SETFL,O_NONBLOCK);
    char buf[16] = {0};
    const ssize_t n = ::read(client,buf,sizeof(buf));
    if(n!=5 || std::string(buf,5)!="hello"){
        std::cerr<<"FAIL:echo should send hello back\n";
        ::close(client);
        return 1;
    }
    ::close(client);
    std::cout<<"OK\n";
    return 0;
}



    
   