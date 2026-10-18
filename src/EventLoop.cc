#include<EventLoop.h>
#include<Channel.h>
#include<EPollPoller.h>
#include<TimerQueue.h>

#include<cerrno>//系统失败调用的错误码
#include<cstdlib>//提供终止进程函数
#include<sys/epoll.h>//提供epoll
#include<sys/eventfd.h>//提供eventfd
#include<unistd.h>//关掉文件描述符

namespace KeeServer{

namespace{//匿名命名空间 这里的成员只有当前文件可见 私有常量
    constexpr int kPollTimeMs = 10000;//编译期常量 不算运行时才算的。10s
}

EventLoop::EventLoop()
    :threadId_(std::this_thread::get_id())//获取线程id
    ,poller_(std::make_unique<EPollPoller>(this))
    ,timerQueue_(std::make_unique<TimerQueue>(this))
{
    wakeupFd_ = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);//创建一个eventfd,事件实例
    if(wakeupFd_<0){
        std::abort();
    }
    wakeupChannel_ = std::make_unique<Channel>(this,wakeupFd_);//创建一个channel登记卡
    wakeupChannel_->setReadCallback([this](){handleWakeup();});//设置
    wakeupChannel_->enableReading();//登记到epoll兴趣队列
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();//从epoll兴趣队列中摘掉
    wakeupChannel_->remove();
    if(wakeupFd_>=0){
        ::close(wakeupFd_);
    }
    // if(epollfd_>=0){
    //     ::close(epollfd_);
    // }
}

void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}

void EventLoop::loop(){//epoll_wait 
    threadId_ = std::this_thread::get_id();
    quit_=false;
    Poller::ChannelList activeChannels;//实例数组
    while(!quit_){
        activeChannels.clear();//清空实例数组
        poller_->poll(kPollTimeMs,&activeChannels);//等待时间，休眠
        for(Channel* ch : activeChannels){
            ch->handleEvent();
        }
        doPendingFunctors();
    }
}

void EventLoop::quit(){
    quit_=true;
    if(!isInLoopThread()){//不是当前线程调用的quit
        wakeup();//唤醒epoll_wait
    }
}

void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){//当前线程调用
        cb();
    }else{
        queueInLoop(std::move(cb));//不是当前线程调用，加入回调函数队列
    }
}

void EventLoop::queueInLoop(Functor cb){
    {
        std::lock_guard<std::mutex> lock(mutex_);//加锁
        pendingFunctors_.push_back(std::move(cb));//加入回调函数队列
    }
    if(!isInLoopThread() || callingPendingFunctors_){//不是当前线程调用，或者正在处理回调函数队列
        wakeup();//唤醒epoll_wait
    }
}

void EventLoop::wakeup(){//唤醒loop
    uint64_t one = 1;
    ::write(wakeupFd_,&one,sizeof(one));//向eventfd写入数据，唤醒epoll_wait
}

void EventLoop::handleWakeup(){//实际回调函数的内容
    uint64_t one = 1;
    ::read(wakeupFd_,&one,sizeof(one));//读取eventfd的数据，清空唤醒事件
}

void EventLoop::runAt(Timestamp time,Functor cb){
    timerQueue_->addTimer(std::move(cb),time,0.0);
}

void EventLoop::runAfter(double delay,Functor cb){
    runAt(addTime(Timestamp::now(),delay),std::move(cb));
}

void EventLoop::runEvery(double interval,Functor cb){
    timerQueue_->addTimer(std::move(cb),
                        addTime(Timestamp::now(), interval),
                        interval);
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);//交换回调函数队列
    }
    for(const Functor& fn:functors){
        fn();//执行回调函数
    }
    callingPendingFunctors_ = false;
}

}