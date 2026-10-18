#include <EventLoopThread.h>
#include <EventLoop.h>

namespace KeeServer{

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,const std::string& name)
    :loop_(nullptr),
     exiting_(false),
     thread_([this]{threadFunc();},name),//构造线程
     callback_(cb)
{}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop(){//在主线程中调用，启动线程，返回loop指针
    thread_.start();//创建os线程，执行threadFunc，和下面的语句并行执行，start（）,已经在新线程中进行了。
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
       cond_.wait(lock,[this]{return loop_ != nullptr;});//等待新线程创建loop对象
        loop = loop_;
    }
    return loop_;
}

void EventLoopThread::threadFunc(){//新线程的一生
    EventLoop loop;//在新线程中创建一个EventLoop对象

    if(callback_){
        callback_(&loop);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;//写进当前类的成员变量
        cond_.notify_one();//敲门，叫醒正在cond_.wait的主线程
    }
    loop.loop();

    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

}