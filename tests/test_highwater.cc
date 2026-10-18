#include<EventLoop.h>
#include<InetAddress.h>
#include<TcpConnection.h>
#include<TcpServer.h>

#include<arpa/inet.h>
#include<atomic>
#include<chrono>
#include<future>
#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<unistd.h>

namespace{

constexpr size_t kPayload = 64*1024;

int connectTo(uint16_t port){
    int fd = ::socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        return -1;
    }
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = ::htons(port);
    ::inet_pton(AF_INET,"127.0.0.1",&dest.sin_addr);
    if(::connect(fd,reinterpret_cast<sockaddr*>(&dest),sizeof(dest))<0){
        ::close(fd);
        return -1;
    }
    return fd;
}

bool waitUntil(const std::atomic<int>& n,int expect){
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(2);
    while(std::chrono::steady_clock::now()<deadline){
        if(n.load()==expect){
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return n.load()==expect;
}

}

int main(){
    const uint16_t port = 18776;
    KeeServer::EventLoop loop;
    KeeServer::TcpServer server(&loop,KeeServer::InetAddress(port,"127.0.0.1"),"HighWater");

    std::atomic<int> high{0};
    server.setConnectionCallback([&](KeeServer::TcpConnection* conn){
        if(!conn->connected()){
            return;
        }
        conn->setHighWaterMarkCallback([&](KeeServer::TcpConnection*,size_t){
            ++high;
        },1024);
    });
    server.setMessageCallback([](KeeServer::TcpConnection* conn,KeeServer::Buffer* buf){
        buf->retrieveAll();
        int snd = 2048;
        ::setsockopt(conn->fd(),SOL_SOCKET,SO_SNDBUF,&snd,sizeof(snd));
        conn->send(std::string(kPayload,'z'));
    });
    server.start();

    int client = connectTo(port);
    if(client<0){
        std::cerr<<"FAIL: client connect\n";
        return 1;
    }
    int rcv = 2048;
    ::setsockopt(client,SOL_SOCKET,SO_RCVBUF,&rcv,sizeof(rcv));
    if(::write(client,"x",1)!=1){
        std::cerr<<"FAIL: client write\n";
        ::close(client);
        return 1;
    }

    std::promise<void> done;
    std::thread t([&]{
        loop.loop();
        done.set_value();
    });

    if(!waitUntil(high,1)){
        std::cerr<<"FAIL: high water callback should fire when output buffer grows\n";
        loop.quit();
        t.join();
        ::close(client);
        return 1;
    }

    std::string got;
    got.reserve(kPayload);
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(2);
    char tmp[4096];
    while(got.size()<kPayload && std::chrono::steady_clock::now()<deadline){
        const ssize_t n = ::read(client,tmp,sizeof(tmp));
        if(n>0){
            got.append(tmp,static_cast<size_t>(n));
        }else{
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    loop.quit();
    auto fut = done.get_future();
    if(fut.wait_for(std::chrono::seconds(1))!=std::future_status::ready){
        std::cerr<<"FAIL: loop() did not return after quit\n";
        std::abort();
    }
    t.join();
    ::close(client);

    if(got.size()!=kPayload){
        std::cerr<<"FAIL: client should still receive the full payload\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}
