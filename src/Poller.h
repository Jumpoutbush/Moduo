#pragma once

#include "Channel.h"
#include "const.h"
#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
/**
 * @brief
 * 多路事件分发器 IO复用模块 epoll_wait
 */
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller();

    // 给IO复用保留统一接口
    virtual Timestamp poll(int intmeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    bool hasChannel(Channel *channel) const;    // 判断channel是否在当前poller中
    static Poller* newDefaultPoller(EventLoop *loop);   // 获取默认的IO复用poller/epollpoller的具体实现
    

protected:
    // key:value - sockfd:channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_;
};