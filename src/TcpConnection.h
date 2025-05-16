#pragma once

#include "noncopyable.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"
#include "Timestamp.h"

#include <atomic>
#include <memory>
#include <string>

class Channel;
class EventLoop;
class Socket;

/**
 * @brief TcpConnection represents a TCP connection
 * TcpServer -> Acceptor -> new connection, get connfd from accept
 * TcpConnection -> Channel -> Poller -> channel->handleEvent
 */
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    explicit TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    void send(const void* message, size_t len);
    void shutdown();

    // set callback
    void setConnectionCallback(const ConnectionCallback& cb){
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb){
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMark(const HighWaterMarkCallback& cb, size_t highWaterMark){
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb){
        closeCallback_ = cb;
    }

    // connecton ctl
    void connectionEstablished();
    void connectionDestroyed();

private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;   // 当前主机
    const InetAddress peerAddr_;    // 客户端主机

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;       // onMessage
    WriteCompleteCallback writeCompleteCallback_;   // onWriteComplete
    HighWaterMarkCallback highWaterMarkCallback_;    // onHighWaterMark
    CloseCallback closeCallback_;     // onClose

    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};