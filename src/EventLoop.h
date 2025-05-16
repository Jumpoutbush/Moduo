#pragma once
#include "Channel.h"
#include "const.h"
#include "CurrentThread.h"
#include "noncopyable.h"
#include "Timestamp.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

class Channel;
class Poller;
// 事件循环 —— channel & poller(epoll)
// 1 eventloop -- 1 poller -- n channels
class EventLoop : public noncopyable
{
public:
    using Functor = std::function<void()>; // 定义一个类型别名，表示回调函数类型
    EventLoop();    
    ~EventLoop();
    void loop();    // 事件循环
    void quit();    // 退出事件循环
    void runInLoop(Functor cb); // 在当前线程中执行回调函数
    void queueInLoop(Functor cb);   // 在当前线程中执行回调函数，如果不在当前线程中，则将回调函数放入队列中，等待下一次循环执行
    Timestamp pollReturnTime() const {return pollReturnTime_;}

    void wakeup();  // 唤醒事件循环

    // 从channel中获取操作
    void updateChannel(Channel* channel);       // 更新channel
    void removeChannel(Channel* channel);       // 移除channel
    bool hasChannel(Channel* channel) const;    // channel是否存在

    bool isInLoopThread() const {return threadId_ == currentThread::tid();} // 判断当前线程是否是事件循环线程
private:
    using ChannelList = std::vector<Channel*>; 
    
    void handleRead();
    void doPendingFunctors();

    std::atomic_bool looping_;  // CAS
    std::atomic_bool quit_;     // quit loop

    const pid_t threadId_; //  定义一个常量pid_t类型的threadId，用于存储线程ID

    int wakeupFd_; //  定义一个整型变量wakeupFd_，用于存储唤醒文件描述符
    std::unique_ptr<Channel> wakeupChannel_; //  定义一个std::unique_ptr<Channel>类型的wakeupChannel_，用于存储唤醒通道

    Timestamp pollReturnTime_; //  定义一个Timestamp类型的pollReturnTime_，用于存储轮询返回时间
    std::unique_ptr<Poller> poller_; //  定义一个std::unique_ptr<Poller>类型的poller_，用于存储轮询器

    ChannelList activeChannels_; //  定义一个ChannelList类型的activeChannels_，用于存储活跃通道列表
    Channel* currentActiveChannel_; //  定义一个Channel*类型的currentActiveChannel_，用于存储当前活跃通道

    std::atomic_bool callingPendingFunctors_;   // loop是否有回调
    std::vector<Functor> pendingFunctors_;  // loop需要执行的所有callback
    std::mutex mutex_;                      // protect pendingFunctors_
};