#pragma once

#include<EventLoop.h>
#include<EventLoopThread.h>

#include<functional>
#include<memory>
#include<string>
#include<vector>

namespace KeeServer{

class EventLoop;

class EventLoopThreadPool{
public:
    using ThreadInitialCallback = EventLoopThread::ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);//构造函数
    ~EventLoopThreadPool() = default;

    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;

    void setThreadNum(int numThreads){numThreads_ = numThreads;}//改变线程池中io线程的数量
    void start(const ThreadInitialCallback& cb = ThreadInitialCallback());//启动线程池，创建io线程和loop对象。按照数字顺序，轮询获取io线程的loop对象
    EventLoop* getNextLoop();//轮询获取io线程的loop对象，给新连接分配io线程
private:
    EventLoop* baseLoop_;//主线程的eventloop
    std::string name_;//线程池名字
    bool started_;//线程池是否启动
    int numThreads_;//线程池中io线程的数量
    int next_;//轮询下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//真正的线程池对象
    std::vector<EventLoop*> loops_;//线程池中的线程的eventloop对象
};
}