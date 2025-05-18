#pragma once

#include "Channel.h"
#include "const.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "noncopyable.h"
#include "Socket.h"

class InetAddress;
class EventLoop;
class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback cb)
    {
        newConnectionCallback_ = cb;
    }
    bool listenning() const {return listenning_;}
    void listen();

private:
    void handleRead();

private:
    EventLoop* loop_;
    Socket acceptSocket_;   // socket在channel之前构造
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};