#include "TcpServer.h"
#include <assert.h>
#include <string.h>

EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop not exists \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
    

TcpServer::TcpServer(EventLoop* loop, 
        const InetAddress& listenAddr,
        const std::string& nameArg,
        Option option) : 
            loop_(CheckLoopNotNull(loop)),
            ipPort_(listenAddr.toIpPort()),
            name_(nameArg),
            acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
            threadPool_(new EventLoopThreadPool(loop_, name_)),
            connectionCallback_(),
            messageCallback_(),
            nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
            std::placeholders::_1, std::placeholders::_2)); // this的tcpserver的newconnection
}

TcpServer::~TcpServer()
{
  LOG_INFO("TcpServer::~TcpServer [%s] destructing", name_.c_str());

  for (auto& item : connections_)
  {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runInLoop(
      std::bind(&TcpConnection::connectionDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));    // unique_ptr acceptor_的listen
}

void TcpServer::start()
{
    if(started_++ == 0) // start多次
    {
        threadPool_->start();
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    LOG_INFO("sockfd = %d", sockfd);
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++ nextConnId_;
    std::string connName = name_ + buf; // name_是TcpServer的名字，buf是连接id
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s",
            name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // 通过sockfd获取绑定的本机ip和port
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) == -1)
    {
        LOG_ERROR("TcpServer::newConnection getsockname error");
    }
    InetAddress localAddr(local);
    assert(sockfd >= 0);
    TcpConnectionPtr conn(
        new TcpConnection(ioLoop,
                          connName,
                          sockfd,
                          localAddr,
                          peerAddr)
    );
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置连接关闭的回调函数
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    // 直接调用TcpConnection::connectionEstablished
    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectionEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - %s", conn->name().c_str(), conn->peerAddress().toIpPort().c_str());
    size_t n = connections_.erase(conn->name());
    (void)n;
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectionDestroyed, conn));
}