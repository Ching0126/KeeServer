#pragma once

#include<functional>
#include<memory>
#include<string>
#include<thread>

namespace KeeServer{

class Thread{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func,const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started()const{return started_;}

private:
    bool started_;//是否开始
    bool joined_;//是否回收
    ThreadFunc func_;//线程函数
    std::string name_;//线程名字
    std::unique_ptr<std::thread> thread_;//线程对象
};
}