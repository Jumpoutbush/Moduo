#pragma once

#include "const.h"
#include "EventLoop.h"
#include "Logger.h"
#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>

class EventLoop;
class Timestamp;
/**
 * @brief 
 * 通道 封装sockfd和event
 * 绑定poller返回的具体事件
 */
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;    // 只读事件

    Channel(EventLoop *loop, int fd);       // loop: Channel所属的EventLoop
    ~Channel();

    void handleEvent(Timestamp receiveTime);    // fd得到poller通知以后调用事件

    // set回调的函数对象
    void setReadCallback(ReadEventCallback cb){readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_ = std::move(cb);}

    // 防止channel被手动remove掉，channel此时还在回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}
    int events() const { return events_;}
    void set_revents(int revt) {revents_ = revt;}
    bool isNoneEvent() const {return events_ == kNoneEvent;}

    // 设置感兴趣的事件，对应callback
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting() {events_ |= kWriteEvent; update();}
    void disableWriting() {events_ &= kWriteEvent; update();}
    void disableAll() {events_ = kNoneEvent; update();}
    bool isWriting() const {return events_ & kWriteEvent;}
    bool isReading() const {return events_ & kReadEvent;}
    int index() {return index_;}
    void set_index(int idx) {index_ = idx;}

    EventLoop* ownerLoop() {return loop_;}
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;    // 当前fd无感兴趣事件
    static const int kReadEvent;    // read
    static const int kWriteEvent;   // write

    EventLoop *loop_;   // 事件循环
    const int fd_;      // poller监听的fd
    int events_;        // 注册fd感兴趣事件
    int revents_;       // poller返回的具体发生的事件
    int index_;         // poller使用

    std::weak_ptr<void> tie_;
    bool tied_;

    // channel中获知fd最终发生的具体事件revents，负责调用最终回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};