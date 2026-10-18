#include<Timestamp.h>

#include<sys/time.h>

namespace KeeServer{

Timestamp Timestamp::now(){//将刺客墙钟时间变成timestamp成员变量。
    timeval tv{};
    /*
    • tv_sec：从 1970-01-01 UTC 起的秒
    • tv_usec：当前这一秒里已经过了多少微秒（0～999999）
    */
    ::gettimeofday(&tv,nullptr);//填入当前时间
    return Timestamp(static_cast<int64_t>(tv.tv_sec)*kMicroSecondsPerSecond+tv.tv_usec);
}
}