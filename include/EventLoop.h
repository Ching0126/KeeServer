#pragma once 

#include<Timestamp.h>

#include<atomic>
#include<functional>
#include<memory>
#include<mutex>
#include<thread>
#include<vector>

namespace KeeServer{

class Channel;
class Poller;
class TimerQueue;

class EventLoop{
public:
    //回调函数类型
    using Functor = std::function<void()>;

    EventLoop();//申请eopll
    ~EventLoop();//关闭epoll

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void loop();
    void quit();

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    //三个定时器函数
    void runAt(Timestamp time,Functor cb);
    void runAfter(double dalay,Functor cb);
    void runEvery(double interval,Functor cb);

    //判断是否在当前线程
    bool isInLoopThread()const{return threadId_ == std::this_thread::get_id();}

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
private:
    void wakeup();//唤醒epoll_wait
    void handleWakeup();//处理唤醒事件
    void doPendingFunctors();//处理回调函数

    std::atomic<bool> quit_{false};//原子bool值 
    std::thread::id threadId_;//线程id
    int wakeupFd_ {-1};//唤醒fd 跨线程门铃
    std::unique_ptr<Channel> wakeupChannel_;//唤醒fd的channel
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;//定时器队列
    std::atomic<bool> callingPendingFunctors_{false};//是否正在处理回调函数
    std::vector<Functor> pendingFunctors_;//回调函数队列
    std::mutex mutex_;//互斥锁
};
}