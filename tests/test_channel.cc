#include<Channel.h>
#include<EventLoop.h>

#include<cstdlib>
#include<iostream>
#include<sys/epoll.h>
#include<unistd.h>

int main(){
    int fds[2];
    if(::pipe(fds)<0){
        std::abort();
    }

    KeeServer::EventLoop loop;
    KeeServer::Channel ch(&loop,fds[0]);
    //三个计时器，调用一次 ++；
    int reads = 0;
    int writes = 0;
    int closes = 0;
    ch.setReadCallback([&]{++reads;});//& 引用外面的read
    ch.setWriteCallback([&]{++writes;});
    ch.setCloseCallback([&]{++closes;});
    
    ch.enableReading();//设置状态变量
    if(!ch.isReading()||ch.isWriting()){
        std::cerr<<"FAIL:enableReading should set read bit only\n";
        return 1;
    }

    ch.handleEvent();
    if(reads!=0){
        std::cerr<<"FAIL:handleEvent must ont fire without revents\n";
        return 1;
    }

    ch.set_revents(EPOLLIN);
    ch.handleEvent();
    if(reads !=1 || writes != 0 || closes != 0){
        std::cerr<<"FAIL: EPOLLIN should fire read only\n";
        return 1;
    }

    ch.set_revents(EPOLLOUT);
    ch.handleEvent();
    if(writes !=1 ||reads != 1){
        std::cerr<<"FAIL: EPOLLOUT should fire write only\n";
        return 1;
    }

    ch.set_revents(EPOLLHUP);
    ch.handleEvent();
    if(closes!=1){
        std::cerr<<"FAIL: EPOLLHUP without EPOLLIN should fire close\n";
        return 1;
    }

    std::cout<<"OK\n";
    return 0;
}