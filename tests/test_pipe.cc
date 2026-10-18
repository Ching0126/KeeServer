#include <Channel.h>
#include<EventLoop.h>

#include<chrono>
#include<cstdlib>
#include<future>
#include<iostream>
#include<thread>
#include<unistd.h>

int main(){
{
    int fds[2];//管道的读写两端 0:读端； 1：写端
    if(::pipe(fds)<0){//造管道
        std::abort();
    }

    KeeServer::EventLoop loop;
    KeeServer::Channel ch(&loop,fds[0]);//这个channel由loop管 并且只负责fds[0]

    int nread = 0;
    ch.setReadCallback([&]{
        char c;
        ::read(fds[0],&c,1);//从哪读 读出来的数据存放在哪 读几个字节
        ++nread;//计数 成功读到几次
        loop.quit();
    });
    ::write(fds[1],"x",1);
}
    
{
    int fds[2];
    if(::pipe(fds)<0){
        std::abort();
    }

    KeeServer::EventLoop loop;
    KeeServer::Channel ch(&loop,fds[0]);
    int nread = 0;
    ch.setReadCallback([&]{
        char c;
        ::read(fds[0],&c,1);
        ++nread;
        loop.quit();
    });
    ch.enableReading();
    ::write(fds[1],"x",1);
    
    loop.loop();
    if(nread !=1){
        std::cerr<<"FAIL: enableReading + pipe wirte should fire read\n";
        return 1;
    }
    ::close(fds[0]);
    ::close(fds[1]);
}
    std::cout<<"OK\n";
    return 0;
}