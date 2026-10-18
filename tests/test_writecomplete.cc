#include<EventLoop.h>
#include<InetAddress.h>
#include<TcpConnection.h>
#include<TcpServer.h>

#include<arpa/inet.h>
#include<atomic>
#include<chrono>
#include<fcntl.h>
#include<future>
#include<iostream>
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
    dest.sin_port = ::htons(port);
    ::inet_pton(AF_INET,"127.0.0.1",&dest.sin_addr);
    if(::connect(fd,reinterpret_cast<sockaddr*>(&dest),sizeof(dest))<0){
        ::close(fd);
        return -1;
    }
    return fd;
}

bool waitUntil(const std::atomic<int>& n,int expect){
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(1);
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
    const uint16_t port = 18775;
    KeeServer::EventLoop loop;
    KeeServer::TcpServer server(&loop,KeeServer::InetAddress(port,"127.0.0.1"),"WriteComplete");

    std::atomic<int> complete{0};
    server.setWriteCompleteCallback([&](KeeServer::TcpConnection*){
        ++complete;
    });
    server.setConnectionCallback([](KeeServer::TcpConnection* conn){
        if(conn->connected()){
            conn->send("hello");
        }
    });
    server.start();

    int client = connectTo(port);
    if(client<0){
        std::cerr<<"FAIL: client connect\n";
        return 1;
    }

    std::promise<void> done;
    std::thread t([&]{
        loop.loop();
        done.set_value();
    });

    if(!waitUntil(complete,1)){
        std::cerr<<"FAIL: writeComplete should fire after send\n";
        loop.quit();
        t.join();
        ::close(client);
        return 1;
    }

    ::fcntl(client,F_SETFL,O_NONBLOCK);
    std::string got;
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(1);
    while(std::chrono::steady_clock::now()<deadline){
        char buf[16] = {};
        const ssize_t n = ::read(client,buf,sizeof(buf));
        if(n>0){
            got.append(buf,static_cast<size_t>(n));
            if(got=="hello"){
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    loop.quit();
    auto fut = done.get_future();
    if(fut.wait_for(std::chrono::seconds(1))!=std::future_status::ready){
        std::cerr<<"FAIL: loop() did not return after quit\n";
        std::abort();
    }
    t.join();
    ::close(client);

    if(got!="hello"){
        std::cerr<<"FAIL: client should receive hello\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}
