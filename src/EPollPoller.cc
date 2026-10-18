#include<EPollPoller.h>
#include<Channel.h>

#include<cerrno>
#include<cstdlib>
#include<unistd.h>

namespace KeeServer{

namespace{
constexpr int kNew = -1;
constexpr int kAdded = 1;
constexpr int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    :Poller(loop)
    ,epollfd_(::epoll_create1(EPOLL_CLOEXEC))//创建epoll实例
    ,events_(kInitEventListSize)//16
{
    if(epollfd_ <0){
        std::abort();
    }
}

EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

int EPollPoller::poll(int timeoutMs,ChannelList* activeChannels){
    const int numEvents = ::epoll_wait(
        epollfd_,events_.data(),static_cast<int>(events_.size()),timeoutMs);
    //监听epoll实例，填入数据的地址，填入数据的大小，最多睡多久。
    if(numEvents > 0){
        fillActiveChannels(numEvents,activeChannels);
        if(static_cast<size_t>(numEvents)==events_.size()){
            events_.resize(events_.size()*2);
        }
    }else if(numEvents <0 && errno != EINTR){
        std::abort();
    }
    return numEvents;
}

void EPollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels)const{//将内核将填好的就绪数组翻译成eventloop挨个处理channel列表
    for(int i=0;i<numEvents;++i){
        Channel* ch = static_cast<Channel*>(events_[i].data.ptr);
        ch->set_revents(events_[i].events);//就绪掩码
        activeChannels->push_back(ch);//放入就绪名单
    }
}

void EPollPoller::updateChannel(Channel* channel){//更新登记卡
    const int index = channel->index();
    if(index == kNew || index == kDeleted){//
        if(index == kNew){
            channels_[channel->fd()] = channel;//列表中deleted的登记卡还在，知识状态被改成了delete，从未来过的需要重新登记
        }
        channel->set_index(kAdded);//改成已经进入
        update(EPOLL_CTL_ADD,channel);
    }else{
        if(channel->isNoneEvent()){//兴趣列表是否为空，是的话，标记为删除，踢出。
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }else{//更新状态
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel){//
    channels_.erase(channel->fd());
    if(channel->index()==kAdded){//在内核里，直接更新为删除
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);//再更新为从未来过
}

void EPollPoller::update(int operation,Channel* channel){//更新内核
    epoll_event ev{};
    ev.events = static_cast<uint32_t>(channel->events());
    ev.data.ptr = channel;
    if(::epoll_ctl(epollfd_,operation,channel->fd(),&ev)<0){
        std::abort();
    }
}
}