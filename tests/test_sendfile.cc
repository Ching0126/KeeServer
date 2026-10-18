#include<EventLoop.h>
#include<InetAddress.h>
#include<TcpConnection.h>
#include<TcpServer.h>

#include<arpa/inet.h>
#include<chrono>
#include<fcntl.h>
#include<fstream>
#include<future>
#include<iostream>
#include<sys/socket.h>
#include<sys/stat.h>
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

}

int main(){
    const uint16_t port = 18777;
    const std::string path = "keeserver_sendfile.txt";
    const std::string payload = "file-payload-from-sendfile";
    {
        std::ofstream out(path,std::ios::binary|std::ios::trunc);
        out << payload;
    }
    const int filefd = ::open(path.c_str(),O_RDONLY);
    if(filefd<0){
        std::cerr<<"FAIL: open temp file\n";
        return 1;
    }

    KeeServer::EventLoop loop;
    KeeServer::TcpServer server(&loop,KeeServer::InetAddress(port,"127.0.0.1"),"SendFile");
    server.setMessageCallback([filefd](KeeServer::TcpConnection* conn,KeeServer::Buffer* buf){
        buf->retrieveAll();
        struct stat st{};
        ::fstat(filefd,&st);
        conn->sendFile(filefd,0,static_cast<size_t>(st.st_size));
    });
    server.start();

    int client = connectTo(port);
    if(client<0){
        std::cerr<<"FAIL: client connect\n";
        ::close(filefd);
        return 1;
    }
    if(::write(client,"x",1)!=1){
        std::cerr<<"FAIL: client write\n";
        ::close(client);
        ::close(filefd);
        return 1;
    }

    std::promise<void> done;
    std::thread t([&]{
        loop.loop();
        done.set_value();
    });

    ::fcntl(client,F_SETFL,O_NONBLOCK);
    std::string got;
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(1);
    while(std::chrono::steady_clock::now()<deadline){
        char buf[64] = {};
        const ssize_t n = ::read(client,buf,sizeof(buf));
        if(n>0){
            got.append(buf,static_cast<size_t>(n));
            if(got==payload){
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
    ::close(filefd);
    ::unlink(path.c_str());

    if(got!=payload){
        std::cerr<<"FAIL: sendFile should deliver the file contents\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}
