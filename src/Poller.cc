#include<Poller.h>
#include<Channel.h>

namespace KeeServer{
Poller::Poller(EventLoop* loop)
    :ownerLoop_(loop)
{}

bool Poller::hasChannel(Channel* channel)const{//寻找是否已登记channel实例
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}
}