#pragma once

#include "const.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Logger.h"
#include "noncopyable.h"

#include <functional>

/**
 * @brief EventLoopThreadPool
 * 负责创建线程池，并将任务分发到线程池中的线程中执行
 * 
 */
class EventLoop;
class EventLoopThread;
class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());
    /// round-robin
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const
    { return started_; }

    const std::string& name() const
    { return name_; }
private:

    EventLoop *baseLoop_;
    std::string name_;
    int numThreads_;
    bool started_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};