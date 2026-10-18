#pragma once

#include<cstdint>

namespace KeeServer{

class Timestamp{
public:
    static const int64_t kMicroSecondsPerSecond = 1000 * 1000;//静态常量 秒和微秒换算尺

    Timestamp()
        :microSecondsSinceEpoch_(0)
    {}

    explicit Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {}

    static Timestamp now();//此刻时间的微妙形式

    int64_t microSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}
private:
    int64_t microSecondsSinceEpoch_;//从 Unix 纪元（1970-01-01 00:00:00 UTC）到这一刻，过了多少微秒。
};

inline bool operator<(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch()<rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs,Timestamp rhs){
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline Timestamp addTime(Timestamp timestamp,double seconds){
    const int64_t delta =
        static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);//换算成微秒
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
//inline 标在函数上，主要是告诉链接器：这个函数的定义允许出现在多个 .cc 里，不要报「重复定义」。
//写在头文件，但不属于这个类
}