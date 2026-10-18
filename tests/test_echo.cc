#include<EchoServer.h>
#include<EventLoop.h>
#include<InetAddress.h>

#include<arpa/inet.h>
#include<chrono>
#include<fcntl.h>
#include<future>
#include<iostream>
#include<sys/socket.h>
#include<thread>
#include<unistd.h>

namespace{

int connectTo(uint16_t port) {//客户端连接函数
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = ::htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&dest), sizeof(dest)) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

}

int main(){
    const uint16_t port = 18767;
    KeeServer::EventLoop loop;
    KeeServer::EchoServer server(&loop,KeeServer::InetAddress(port,"127.0.0.1"),"Echo");
    server.start();

    int client = connectTo(port);
    if(client<0){
        std::cerr<<"FAIL:client write\n";
        return 1;
    }
    if (::write(client, "hello", 5) != 5) {
        std::cerr << "FAIL: client write\n";
        ::close(client);
        return 1;
    }

    std::promise<void> done;
    std::thread t([&]{
        loop.loop();
        done.set_value();
    });

    ::fcntl(client,F_SETFL,O_NONBLOCK);//客户端改成非阻塞
    std::string got;
    const auto deadline = 
        std::chrono::steady_clock::now() + std::chrono::seconds(1);//最多等待1s
    while(std::chrono::steady_clock::now()<deadline){
        char buf[16] = {};
        const ssize_t n = ::read(client,buf,sizeof(buf));
        if(n>0){
            got.append(buf,static_cast<size_t>(n));
            if(got == "hello"){
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    loop.quit();
    auto fut = done.get_future();
    if(fut.wait_for(std::chrono::seconds(1))!=std::future_status::ready){
        std::cerr<<"FAIL :loop()did not return after quit\n";
        std::abort();
    }
    t.join();
    ::close(client);

    if(got!="hello"){
        std::cerr<<"FAIL:EchoServer should send hello back\n";
        return 1;
    }

    std::cout<<"ok\n";
    return 0;
}