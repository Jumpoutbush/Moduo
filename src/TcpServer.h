#pragma once

#include "Acceptor.h"
#include "Callbacks.h"
#include "const.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Logger.h"
#include "noncopyable.h"
#include "TcpConnection.h"

#include <atomic>
#include <functional>

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
private:
    ConnectionCallback connectionCallback_; // newConnection
    MessageCallback messageCallback_;       // onMessage
    ThreadInitCallback threadInitCallback_; // new thread
    WriteCompleteCallback writeCompleteCallback_;   // onWriteComplete
public:
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };
    TcpServer(
        EventLoop* loop,
        const InetAddress& listenAddr,
        const std::string& nameArg,
        Option option = kNoReusePort);
    ~TcpServer();

    /// Set the number of threads for handling input.
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb) { 
        threadInitCallback_ = cb; 
    }
    /// Set connection callback. Not thread safe.
    void setConnectionCallback(const ConnectionCallback& cb) { 
        connectionCallback_ = cb; 
    }
    /// Set message callback. Not thread safe.
    void setMessageCallback(const MessageCallback& cb){ 
        messageCallback_ = cb; 
    }
    /// Set write complete callback. Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { 
        writeCompleteCallback_ = cb; 
    }

    /// valid after calling start()
    std::shared_ptr<EventLoopThreadPool> threadPool(){ 
        return threadPool_; 
    }

    /// Starts the server if it's not listening. 
    /// harmless to call it multiple times.
    /// Thread safe.
    void start();
private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd, const InetAddress& peerAddr);
    /// Thread safe.
    void removeConnection(const TcpConnectionPtr& conn);
    /// Not thread safe, but in loop
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    using ConnectionMap = std::unordered_map<std::string, std::shared_ptr<TcpConnection>>;
   
    EventLoop *loop_;
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_; // connId -> conn
};