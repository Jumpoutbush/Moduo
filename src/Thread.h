#pragma once

#include "const.h"
#include "CurrentThread.h"
#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <thread>

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();
    void detach();
    bool started() const { return started_; }
    bool joined() const { return joined_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }
    static pid_t gettid();
    static std::string to_string(pid_t tid);
    void setDefaultName();

private:
    bool started_;
    bool joined_;   
    // std::thread thread_;    // 会直接启动
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;         // 线程id
    ThreadFunc func_;   // 线程函数
    std::string name_;  // 线程名字
    static std::atomic_int32_t numCreated_;  // 线程创建计数
};