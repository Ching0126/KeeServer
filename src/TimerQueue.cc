#include<TimerQueue.h>
#include<EventLoop.h>
#include<Timer.h>

#include<algorithm>
#include<cstdint>
#include<cstring>
#include<iterator>
#include<sys/timerfd.h>
#include<unistd.h>

namespace KeeServer{
namespace{
int createTimerfd(){//创造定时器fd
    int fd = ::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);
    if(fd < 0){
        std::abort();
    }
    return fd;
}
void readTimerfd(int timerfd){
    //到期后内核往这个 fd 里写一个 uint64_t（超时了几次）。必须读走，否则 epoll 会认为一直可读，、
    // handleRead 空转。howmany 读完就丢，你们用用户态 timers_ 判断谁到期，不靠这个计数。
    uint64_t howmany = 0;
    ::read(timerfd,&howmany,sizeof(howmany));
}
}

TimerQueue::TimerQueue(EventLoop* loop)
    :loop_(loop)
    ,timerfd_(createTimerfd())
    ,timerfdChannel_(loop,timerfd_)//创建定时器登记卡
{
    timerfdChannel_.setReadCallback([this]{handleRead();});
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue(){
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for(const Entry& entry : timers_){
        delete entry.second;
    }
}

void TimerQueue::addTimer(TimerCallback cb,Timestamp when,double interval){
    Timer* timer = new Timer(std::move(cb),when,interval);
    loop_->runInLoop([this,timer]{addTimerInLoop(timer);});
}

void TimerQueue::addTimerInLoop(Timer* timer){
    const bool earlistChanged = insert(timer);
    if(earlistChanged){
        resetTimerfd(timer->expiration());//更新闹钟的过期时间
    }
}

void TimerQueue::resetTimerfd(Timestamp expiration){
    itimerspec newValue{};
    int64_t micros = expiration.microSecondsSinceEpoch()- 
                    Timestamp::now().microSecondsSinceEpoch();//新闹钟差值
    if(micros <100){
        micros = 100;
    }
    newValue.it_value.tv_sec = 
        static_cast<time_t>(micros/Timestamp::kMicroSecondsPerSecond);
    newValue.it_value.tv_nsec = static_cast<long>(
        (micros % Timestamp::kMicroSecondsPerSecond) * 1000);//纳秒
    ::timerfd_settime(timerfd_,0,&newValue,nullptr);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){//将所有的过期闹钟取下
    std::vector<Entry> expired;
    const Entry sentry(now,reinterpret_cast<Timer*>(UINTPTR_MAX));
    const auto end = timers_.lower_bound(sentry);//获取到第一个还没到期的闹钟
    std::copy(timers_.begin(),end,std::back_inserter(expired));
    timers_.erase(timers_.begin(),end);
    return expired;
}

void TimerQueue::handleRead(){
    const Timestamp now = Timestamp::now();
    readTimerfd(timerfd_);
    const std::vector<Entry> expired = getExpired(now);//获取过期闹钟
    for(const Entry& entry : expired){//跑回调
        entry.second->run();
    }
    reset(expired,now);//重置
}

void TimerQueue::reset(const std::vector<Entry>& expired,Timestamp now){
    for(const Entry& entry : expired){
        if(entry.second->repeat()){
            entry.second->restart(now);
            insert(entry.second);
        }else{
            delete entry.second;
        }
    }
    if(!timers_.empty()){
        resetTimerfd(timers_.begin()->second->expiration());
    }
}

bool TimerQueue::insert(Timer* timer){
    const Timestamp when = timer->expiration();
    const bool earlistChanged = //对头元素有无被换掉
        timers_.empty() || when < timers_.begin()->first;//列表为空，或者当前闹钟比队列最早闹钟还早
    timers_.insert(Entry(when,timer));
    return earlistChanged;
}
}