#pragma once 

#include<Poller.h>

#include<sys/epoll.h>
#include<vector>

namespace KeeServer{

class EPollPoller : public Poller{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    int poll(int timeoutMs,ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
private:
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;//写入就绪事件
    void update(int operation,Channel* channel);//更新掩码

    int epollfd_;//epoll实例
    std::vector<epoll_event> events_;//epoll事件，这里的数据都是内核往里面天填的。
};
}