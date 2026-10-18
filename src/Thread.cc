#include<Thread.h>

namespace KeeServer{

Thread::Thread(ThreadFunc func,const std::string& name)
    : started_(false),
      joined_(false),
      func_(func),
      name_(name)
{}

Thread::~Thread(){
    if(started_ && !joined_){
        thread_->detach();//分离线程，线程结束后自动回收资源
    }
}

void Thread::start(){
    started_ = true;
    thread_ = std::make_unique<std::thread>(func_);//创建线程，执行func_
}

void Thread::join(){//等待线程结束
    joined_ = true;
    thread_->join();
}

}