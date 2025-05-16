#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "TcpConnection.h"

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
TcpConnection::TcpConnection(
        EventLoop* loop, 
        const std::string& name, 
        int sockfd, 
        const InetAddress& localAddr, 
        const InetAddress& peerAddr )
        : loop_(CheckLoopNotNull(loop))
        , name_(name)
        , state_(kConnecting)
        , reading_(true)
        , socket_(new Socket(sockfd))
        , channel_(new Channel(loop, sockfd))
        , localAddr_(localAddr)
        , peerAddr_(peerAddr)
        , highWaterMark_(64*1024*1024)  // 64MB
{
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG("pConnection::ctor[%s] at %p fd = %d\n", name_.c_str(), this, sockfd);
    socket_->setKeepAlive(true);
}
TcpConnection::~TcpConnection()
{
    LOG_DEBUG("TcpConnection::dtor[%s] at %p fd = %d state = %s\n", name_.c_str(), this, channel_->fd(), stateToString());
}
void TcpConnection::send(const void* message, size_t len)
{

}
void TcpConnection::shutdown()
{

}
void TcpConnection::connectionEstablished()
{

}
void TcpConnection::connectionDestroyed()
{

}
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if(n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)     // 对方关闭连接
    {
        handleClose();
    }   
    else // error
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)      // 如果设置了回调函数，则调用回调函数
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_DEBUG("TcpConnection::handleWrite fd=%d is down, disable writing", channel_->fd());
    }
}

void TcpConnection::handleClose()
{

}
void TcpConnection::handleError()
{

}
void TcpConnection::sendInLoop(const void* message, size_t len)
{

}
void TcpConnection::shutdownInLoop()
{

}