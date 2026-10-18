//一条线程里跑且只跑一个 EventLoop。
#pragma once

#include<condition_variable>
#include<functional>
#include<mutex>
#include<string>
#include<Thread.h>

namespace KeeServer{

class EventLoop;

class  EventLoopThread{ 
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),const std::string& name = std::string());
    ~EventLoopThread();

    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;

    EventLoop* startLoop();//启动线程，返回loop指针
private:
    void threadFunc();//新线程入口，为startLoop服务

    EventLoop* loop_;//事件循环，需要回收
    bool exiting_;//是否退出
    Thread thread_;//工作线程，需要回收
    std::mutex mutex_;//互斥锁
    std::condition_variable cond_;//条件变量
    ThreadInitCallback callback_;//线程初始化回调函数
};
}