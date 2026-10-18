#include<EventLoopThreadPool.h>
#include<EventLoop.h>

#include<cstdio>

namespace KeeServer{

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg)
    :baseLoop_(baseLoop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{}

void EventLoopThreadPool::start(const ThreadInitialCallback& cb){
    started_ = true;
    for(int i=0;i<numThreads_;++i){
        char buf[32];
        std::snprintf(buf,sizeof(buf),"%s%d",name_.c_str(),i);
        auto t = std::make_unique<EventLoopThread>(cb,buf);//创建io线程对象
        EventLoop* loop = t->startLoop();
        loops_.push_back(loop);//放入线程池的eventloop对象数组中
        threads_.push_back(std::move(t));//放入线程池对象数组中
    }
    if(numThreads_ == 0 && cb){
        cb(baseLoop_);//如果没有io线程，直接在主线程中调用回调函数
    }
}

EventLoop* EventLoopThreadPool::getNextLoop(){
    EventLoop* loop = baseLoop_;//默认为beaseLoop_，即主线程的eventloop
    if(!loops_.empty()){
        loop = loops_[next_];
        ++next_;
        if(static_cast<size_t>(next_) >= loops_.size()){
            next_ = 0;
        }
    }
    return loop;
}

}