#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include <errno.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

thread_local EventLoop* t_loopInThisThread = 0; // 线程局部存储，防止一个线程创建多个eventloop

const int kPollTimeoutMs = 10000;   // Poller轮询超时时间10s

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error: %d\n", errno);
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(currentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));   // 告诉wakeupchannel，有数据可读时，应该调用eventloop对象的handleRead函数
    wakeupChannel_->enableReading(); // 注册wakeupfd的读事件
}
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping\n", this);
    while(!quit_)
    {
        activeChannels_.clear();
        // poller监听哪些channel发生事件了，上报给eventloop，通知channel处理
        // epollwait 阻塞 kPollTimeoutMs
        pollReturnTime_ = poller_->poll(kPollTimeoutMs, &activeChannels_);  // 往activeChannels_中添加就绪事件

        for(Channel* channel : activeChannels_)
        {
            channel->handleEvent(pollReturnTime_);
        }
        /**
         *  执行当前eventloop事件循环要处理的回调任务
         *  IO线程mainloop accept fd <= channel subloop
         */ 
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
    looping_ = false;
}
void EventLoop::quit()
{
    quit_ = true;
    // 其他线程调用quit，唤醒当前loop线程，让其退出loop
    if(!isInLoopThread())
    {
        wakeup();
    }
}
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else    // 在非当前线程中执行cb
    {
        queueInLoop(cb);
    }
}

// 在非当前线程中执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    // 唤醒loop线程，执行cb
    if(!isInLoopThread() || callingPendingFunctors_)    // 当前loop线程不在loop，或者loop线程正在执行回调任务
    {
        wakeup();
    }
}
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));    // wakeupFd_(createEventfd())
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel* channel) const
{
    return poller_->hasChannel(channel);
}
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", (int)n);
    }
}
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor& functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;
}