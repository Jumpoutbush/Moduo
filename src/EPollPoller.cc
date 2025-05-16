#include "Channel.h"
#include "EPollPoller.h"
#include "Logger.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

// channel未添加到poller中
const int kNew = -1;    // channel成员index_初始化-1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if(epollfd_ < 0){
        LOG_FATAL("epoll_ create error:%d \n", errno);
    }
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}
Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_DEBUG("func = %s => fd total count %d\n", __FUNCTION__, channels_.size());
    int numEvents = ::epoll_wait(epollfd_, 
        &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0)
    {
        LOG_DEBUG("%s has %d events happened \n", __FUNCTION__, numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s nothing happened \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_FATAL("%s EPollPoller::poll()", __FUNCTION__);
        }
    }
    return now;
}

void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)    // success = 0, errno = -1
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl del error:%d\n", errno);
        }
    }
}

/**
 *                      EventLoop
 *      ChannelLst                      Poller
 *                               ChannelMap<fd, channel*>
 */
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd = %d events = %d index = %d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    if(index == kNew || index == kDeleted)  // 当前无注册事件
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else    // 有注册事件
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())      // fd无感兴趣事件
        {
            update(EPOLL_CTL_DEL, channel); 
        }
        else                            // fd有感兴趣事件
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
void EPollPoller::removeChannel(Channel *channel)
{
    // fd op
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func = %s => fd = %d \n", __FUNCTION__, fd);
    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);   // 重置为初始状态
}
// poll中epoll_wait已返回准备好进行IO操作的fd数量，fill把对应的通道加入到活跃列表中
void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for(int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}