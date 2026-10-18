#pragma once 

#include<unordered_map>
#include<vector>

namespace KeeServer{

class Channel;
class EventLoop;

class Poller{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&)= delete;

    virtual int poll(int timesoutMs,ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    bool hasChannel(Channel* channel) const;

protected:
    using ChannelMap = std::unordered_map<int,Channel*>;
    ChannelMap channels_;//实例表
    EventLoop* ownerLoop_;//Poller对应的eventLoop
};
}