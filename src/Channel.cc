#include "Channel.h"

#include <assert.h>
#include <sys/epoll.h>
#include <execinfo.h>
#include <unistd.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;    
const int Channel::kWriteEvent = EPOLLOUT;
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), 
      fd_(fd), 
      events_(0),
      revents_(0), 
      index_(-1),
      tied_(false)
    {
        LOG_INFO("Channel::Channel[%p] fd=%d", this, fd);
    
        if (fd <= 2 || fd >= 100000)
        {
            void* bt[10];
            int n = backtrace(bt, 10);
            fprintf(stderr, "Invalid fd = %d passed to Channel::Channel()\n", fd);
            backtrace_symbols_fd(bt, n, STDERR_FILENO);
            // abort();  // 立即终止并打印调用堆栈
        }
    }

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

/**
 * @brief 
 * 改变channel表示的fd事件后，update在poller里面改变fd对应的事件epoll_ctl
 * EventLoop 包含： ChannelList、Poller 相互独立  
 */
void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

// Define a member function of the Channel class to handle events
void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if(tied_){
        // 检查weak_ptr是否仍然有效
        guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_DEBUG("channel handleEvent revents:%d", revents_);
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}