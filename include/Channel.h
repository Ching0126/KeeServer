#pragma once

#include<functional>

namespace KeeServer{

class EventLoop;

class Channel{
public:
    using EventCallback = std::function<void()>;//一个能装「void 函数()」的盒子。

    Channel(EventLoop* loop,int fd);
    ~Channel() = default;

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    void handleEvent();
    
    //设置四个回调函数，将cb设置为相应的回调函数
    void setReadCallback(EventCallback cb){readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_ = std::move(cb);}

    //获取状态
    int fd()const{return fd_;}
    int events()const{return events_;}
    void set_revents(int revt){revents_ = revt;}

    //设置状态
    void enableReading();//| 按位置位
    void enableWriting();
    void disableWriting();
    void disableAll();
    void remove();

    //判断状态
    bool isNoneEvent()const{return events_==kNoneEvent;}
    bool isWriting()const{return events_&kWriteEvent;}//按位检查
    bool isReading()const{return events_&kReadEvent;}

    int index()const{return index_;}
    void set_index(int idx){index_=idx;}

    EventLoop* ownerLoop(){return loop_;}

private:
    void update();
    //三个常量 将epoll标志起的别名，这里仅作声明 真正赋值在.cc文件中
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    /*
        EPOLLIN：普通可读（有数据、对端关闭时也会可读）
        EPOLLPRI：带外数据，通常和读放一起关心
         EPOLLOUT：可写
    */
    //四个状态
    EventLoop* loop_;
    const int fd_;
    int events_;//兴趣 想监听什么
    int revents_;//实际发生了什么 从epoll_wait抄下来
    int index_;//记录当前channel和eopll的关系 
    
    //四个无参无返回值的回调函数
    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
}