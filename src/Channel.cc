#include<Channel.h>
#include<EventLoop.h>
#include<sys/epoll.h>

namespace KeeServer{
//为三个状态赋值
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop,int fd)//循环事件+文件描述符
    :loop_(loop),
    fd_(fd),
    events_(0),//自己填 想关心什么事件
    revents_(0),//实际发生了什么事件
    index_(-1)//表示当前事件分发器还未进入epoll
    {}
/*
    EPOLLIN     可读
    EPOLLPRI    紧急可读
    EPOLLOUT    可写
    EPOLLERR    出错
    EPOLLHUP    挂断
*/
void Channel::handleEvent(){//只看revents 而不是events 
    if((revents_&EPOLLHUP) && !(revents_ & EPOLLIN)){//对端关闭且没有可读
        if(closeCallback_){
            closeCallback_();
        }
    }
    if(revents_ & EPOLLERR){//错误回调
        if(errorCallback_){
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI)){//普通数据或者外带数据
        if(readCallback_){//有没有登记过？
            readCallback_();
        }
    }
    if(revents_ & EPOLLOUT){//写回调
        if(writeCallback_){
            writeCallback_();
        }
    }
}

void Channel::update(){//更新
    loop_->updateChannel(this);
}

void Channel::enableReading(){
    events_ |= kReadEvent;
    update();
}

void Channel::enableWriting(){
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting(){
    events_ &= ~kWriteEvent;
    update();
}
void Channel::disableAll(){
    events_=kNoneEvent;
    update();
}

void Channel::remove(){
    loop_->removeChannel(this);
}
}