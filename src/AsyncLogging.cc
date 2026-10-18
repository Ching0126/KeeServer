#include<AsyncLogging.h>
#include<LogFile.h>

#include<chrono>

namespace KeeServer{

AsyncLogging::AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval)
    :flushInterval_(flushInterval)
    ,running_(false)
    ,basename_(basename)
    ,rollSize_(rollSize)
    ,thread_([this]{threadFunc();},"Logging")
    ,currentBuffer_(std::make_unique<LargeBuffer>()){
}

AsyncLogging::~AsyncLogging(){
    if(running_){
        stop();
    }
}

void AsyncLogging::start(){
    running_ = true;
    thread_.start();
}

void AsyncLogging::stop(){
    running_ = false;
    cond_.notify_one();//唤醒阻塞的线程
    thread_.join();
}

void AsyncLogging::append(const char* logline,int len){
    std::lock_guard<std::mutex> lock(mutex_);
    if(currentBuffer_->avail()>len){
        currentBuffer_->append(logline,static_cast<size_t>(len));
    }else{
        buffers_.push_back(std::move(currentBuffer_));
        currentBuffer_=std::make_unique<LargeBuffer>();
        currentBuffer_->append(logline,static_cast<ssize_t>(len));
        cond_.notify_one();//条件变量通知
    }
}

void AsyncLogging::threadFunc(){
    LogFile output(basename_,rollSize_,flushInterval_);
    while(true){
        BufferVector buffersToWrite;
        {
            //锁是为了buffer缓冲而加的，防止竞争缓冲
            std::unique_lock<std::mutex> lock(mutex_);
            if(buffers_.empty()&&running_){
                cond_.wait_for(lock,std::chrono::seconds(flushInterval_));
            }
            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_=std::make_unique<LargeBuffer>();
            buffersToWrite.swap(buffers_);
        }
        //只有当前进程可以往文件里面写，所以可以锁外写
        for(const BufferPtr& buf : buffersToWrite){//锁外再将文件慢慢写入
            output.append(buf->data(),static_cast<ssize_t>(buf->length()));
        }
        output.flush();
        if(!running_){//运行停止，则结束循环
            break;
        }
    }
}
}