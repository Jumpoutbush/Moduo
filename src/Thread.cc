#include "Thread.h"

#include <semaphore.h>

std::atomic_int32_t Thread::numCreated_;

void Thread::setDefaultName()
{
    int num = numCreated_.fetch_add(1);
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread-%d", num);
        name_ = buf;
    }
}
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name)
{
    setDefaultName();
}
    
Thread::~Thread()
{
    if(started_ && !joined_)
    {
        pthread_detach(tid_);
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, 0, 0);
    // start a new thread
    // thread_ = std::make_shared<std::thread>([&](){
    //     tid_ = currentThread::tid();
    //     sem_post(&sem);
    //     currentThread::t_cachedTid = tid_;
    //     func_();
    //     currentThread::t_cachedTid = 0;
    //     tid_ = 0;
    // });
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = currentThread::tid();
        sem_post(&sem);
        currentThread::t_cachedTid = tid_;
        func_();
        currentThread::t_cachedTid = 0;
        tid_ = 0;
    }));
    sem_wait(&sem); // wait for the thread to start
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}