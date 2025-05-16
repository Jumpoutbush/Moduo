#include "TcpServer.h"

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
        Option option = kNoReusePort) : 
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

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{

}