//一个eventloop上的闹钟中心。
#pragma once

#include <Channel.h>
#include <Timestamp.h>

#include <functional>
#include <set>
#include <vector>

namespace KeeServer {

class EventLoop;
class Timer;

class TimerQueue {
public:
    using TimerCallback = std::function<void()>;

    //构造函数和析构函数
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;

    void addTimer(TimerCallback cb, Timestamp when, double interval);//对外唯一入口

private:
    using Entry = std::pair<Timestamp, Timer*>;//何时响和对应闹钟的键值对
    using TimerList = std::set<Entry>;//set是有序的

    void addTimerInLoop(Timer* timer);
    void handleRead();
    void resetTimerfd(Timestamp expiration);
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(Timer* timer);

    EventLoop* loop_;//属于哪个值班台
    const int timerfd_;//闹钟fd
    Channel timerfdChannel_;//把timerfd_包装成Channel
    TimerList timers_;//按到期时间从早到晚排，开头永远是最早响的一个。
};

}  // namespace KeeServer