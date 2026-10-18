
#include<Acceptor.h>
#include<EventLoop.h>
#include<InetAddress.h>

#include<arpa/inet.h>
#include<chrono>
#include<future>
#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<unistd.h>


int connectTo(uint16_t port){//客户端建立连接
    int fd = ::socket(AF_INET,SOCK_STREAM,0);//创建fd
    if(fd<0){
        return -1;
    }

    sockaddr_in dest{};//存地址
    dest.sin_family = AF_INET;
    dest.sin_port = ::htons(port);
    ::inet_pton(AF_INET,"127.0.0.1",&dest.sin_addr);
    
    //建立连接
    if(::connect(fd,reinterpret_cast<sockaddr*>(&dest),sizeof(dest))<0){
        ::close(fd);
        return -1;
    }
    
    //写入
    if(::write(fd,"b",1)!=1){
        ::close(fd);
        return -1;
    }
    return fd;
}

int main(){
    const uint16_t port = 18765;
{
    KeeServer::EventLoop loop;
    KeeServer::Acceptor acceptor(&loop,KeeServer::InetAddress(port,"127.0.0.1"));//构造函数完成了创建套接字、bind 以及setruse等工作
    if(acceptor.listenning()){
        std::cerr<<"FAIL:ctor must not listen\n";
        return 1;
    }
    if(connectTo(port)>=0){
        std::cerr<<"FAIL:connect succeeded before listen()\n";
        return 1;
    }
}
{
    KeeServer::EventLoop loop;
    KeeServer::Acceptor acceptor(&loop,KeeServer::InetAddress(port,"127.0.0.1"));

    int accepted = -1;
    KeeServer::InetAddress peer;
    acceptor.setNewConnectionCallback([&](int fd,const KeeServer::InetAddress& addr){
        accepted = fd;
        peer = addr;
        loop.quit();//回调之后，loop退出
    });
    acceptor.listen();
    if(!acceptor.listenning()){
        std::cerr<<"FAIL:listen() should set listenning\n";
        return 1;
    }

    int client = connectTo(port);//客户端连接，并且写入数据
    if(client<0){
        std::cerr<<"FAIL:connect after listen()\n";
        return 1;
    }

    std::promise<void> done;
    std::thread t([&]{
        loop.loop();//等待epoll_wait
        done.set_value();
    });
    auto fut = done.get_future();
    if(fut.wait_for(std::chrono::seconds(1))!=std::future_status::ready){//等待一秒，loop没结束
        std::cerr<<"FAIL:listenfd readable did not reach handleRead\n";
        loop.quit();
        std::abort();
    }
    t.join();//回收线程

    if(accepted<0){
        std::cerr<<"FAIL: callback should receive connfd\n";
        ::close(client);
        return 1;
    }
    if(peer.toIp()!="127.0.0.1"){
        std::cerr<<"FAIL:peer ip\n";
        ::close(accepted);
        ::close(client);
        return 1;
    }

    char c = 0;
    const ssize_t n = ::read(accepted,&c,1);
    if(n != 1 || c!='b'){
        std::cerr<<"FAIL:accepted fd should receive client byte\n";
        ::close(accepted);
        ::close(client);
        return 1;
    }
    ::close(accepted);
    ::close(client);
}

    std::cout<<"OK\n";
    return 0;
}