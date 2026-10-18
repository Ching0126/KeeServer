#include<EventLoop.h>
#include<InetAddress.h>
#include<TcpServer.h>

#include<arpa/inet.h>
#include<chrono>
#include<future>
#include<iostream>
#include<thread>
#include<unistd.h>

namespace{

constexpr size_t kPayload = 64*1024;

int connectTo(uint16_t port){
    int fd = ::socket(AF_INET,SOCK_STREAM,0);
    if(fd < 0){
        std::cerr<<"socket error\n";
        return -1;
    }
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    ::inet_pton(AF_INET,"127.0.0.1",&dest.sin_addr);
    if(::connect(fd,reinterpret_cast<sockaddr*>(&dest),sizeof(dest)) < 0){
        std::cerr<<"connect error\n";
        ::close(fd);
        return -1;
    }
    return fd;
}
}

int main(){
    const uint16_t port = 18768;
    KeeServer::EventLoop loop;
    KeeServer::TcpServer server(&loop,KeeServer::InetAddress(port,"127.0.0.1"),"send");
    server.setMessageCallback([](KeeServer::TcpConnection* conn,KeeServer::Buffer* buf){
        buf->retrieveAll();
        int snd = 2048;
        ::setsockopt(conn->fd(),SOL_SOCKET,SO_SNDBUF,&snd,sizeof(snd));//发送缓冲区最多2KB
        conn->send(std::string(kPayload,'z'));//发送64KB的z
    });
    server.start();

    int client = connectTo(port);
    if(client < 0){
        std::cerr<<"connectTo error\n";
        return 1;
    }

    int rcv = 2048;//2kb
    ::setsockopt(client,SOL_SOCKET,SO_RCVBUF,&rcv,sizeof(rcv));
    if(::write(client,"x",1) != 1){//客户端往自己的套接字写入数据，内核将其传给服务端。
        std::cerr<<"write error\n";
        close(client);
        return 1;
    }

    std::promise<void> done;
    std::thread t([&]{
        loop.loop();
        done.set_value();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string got;
    got.reserve(kPayload);//提前预留64KB的内存容量
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    char tmp[4096];
    while(got.size() < kPayload && std::chrono::steady_clock::now() < deadline){//2s内，将64KB的z传入got
        ssize_t n = ::read(client,tmp,sizeof(tmp));
        if(n > 0){
            got.append(tmp,static_cast<size_t>(n));
        }else{
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    loop.quit();
    auto fut  = done.get_future();
    if(fut.wait_for(std::chrono::seconds(1)) != std::future_status::ready){
        std::cerr<<"Fail: loop() did not return after quit()\n";
        std::abort();
    }
    t.join();
    ::close(client);

    if(got.size() != kPayload || got.find_first_not_of('z') != std::string::npos){
        std::cerr<<"Fail:should receive all 256KB after partial writes\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}