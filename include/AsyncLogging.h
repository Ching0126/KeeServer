#pragma once

#include<LogStream.h>
#include<Thread.h>

#include<atomic>
#include<condition_variable>
#include<memory>
#include<mutex>
#include<string>
#include<sys/types.h>
#include<vector>

namespace KeeServer{

class AsyncLogging{
public:
    AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging();

    AsyncLogging(const AsyncLogging&) = delete;
    AsyncLogging& operator=(const AsyncLogging&) = delete;

    void start();
    void stop();
    void append(const char* logline, int len);
private:
    using LargeBuffer = FixedBuffer<kLargeBufferSize>;//单缓冲
    using BufferPtr = std::unique_ptr<LargeBuffer>;//单缓冲指针
    using BufferVector = std::vector<BufferPtr>;//缓冲数组

    void threadFunc();

    const int flushInterval_;//刷新间隔
    std::atomic<bool> running_;//后台循环需不需要转
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;//线程，跑threadFunc那条
    std::mutex mutex_;//进程锁
    std::condition_variable cond_;//条件变量
    BufferPtr currentBuffer_;//前端真正写的那一块
    BufferVector buffers_;//已写满，等待后端拿走
};
}