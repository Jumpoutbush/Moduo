#include "Channel.h"

#include <assert.h>
#include <sys/epoll.h>

static const int kNoneEvent = 0;    
static const int kReadEvent = EPOLLIN | EPOLLPRI;    
static const int kWriteEvent = EPOLLOUT;
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1),
      tied_(false)
    {}

Channel::~Channel()
{
    assert(!tied_);

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
    if(tied_){
        // 检查weak_ptr是否仍然有效，如果有效则调用handleEventWithGuard，否则什么也不做
        // weak_ptr有效返回shared_ptr，否则返回nullptr(空的shared_ptr)
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // Log the received event flags for debugging purposes
    LOG_INFO("channel handleEvent revents:%d", revents_);

    // Check if the hang up event is received and no input event is pending
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        // If a close callback is registered, execute it
        if(closeCallback_){
            closeCallback_();
        }
    }
    // Check if an error event is received
    if(revents_ & EPOLLERR){
        // If an error callback is registered, execute it
        if(errorCallback_){
            errorCallback_();
        }
    }
    // Check if a read or priority read event is received
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        // If a read callback is registered, execute it with the receive time
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }
    // Check if a write event is received
    if(revents_ & EPOLLOUT){
        // If a write callback is registered, execute it
        if(writeCallback_){
            writeCallback_();
        }
    }
}