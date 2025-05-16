#pragma once

#include "const.h"
#include "Poller.h"
#include "Timestamp.h"

#include <sys/epoll.h>

class Channel;
/**
 * @brief
 * epoll create, ctl增删改, wait
 */
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
private:
    static const int kInitEventListSize = 16;   // epoll_event初始长度

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};

