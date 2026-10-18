//这是一个闹钟
#pragma once 

#include<Timestamp.h>

#include<functional>

namespace KeeServer{

class Timer{
public:
    using TimerCallback = std::function<void()>;

    Timer(TimerCallback cb,Timestamp when,double interval)
        :callback_(std::move(cb))
        ,expiration_(when)
        ,interval_(interval)
        ,repeat_(interval > 0.0)
    {}

    void run() const{callback_();}//运行回调

    Timestamp expiration()const{return expiration_;}//返回到期时间
    bool repeat()const{return repeat_;}//返回是否重复

    void restart(Timestamp now){
        if(repeat_){
            expiration_ = addTime(now,interval_);
        }else{
            expiration_ = Timestamp();
        }
    }
private:
    const TimerCallback callback_;//到点要跑的函数
    Timestamp expiration_;//下次响起的时刻
    const double interval_;//两次之间的间隔
    const bool repeat_;//是不是周期闹钟
};
}