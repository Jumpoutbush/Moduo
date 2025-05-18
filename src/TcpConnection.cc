#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "TcpConnection.h"

#include <assert.h>

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
    LOG_INFO("pConnection::ctor[%s] at %p fd = %d\n", name_.c_str(), this, sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG("TcpConnection::dtor[%s] at %p fd = %d state = %s\n", name_.c_str(), this, channel_->fd(), stateToString());
}

void TcpConnection::send(const std::string& buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())     // 如果当前线程是IO线程，则直接发送
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size())
            );
        }
    }
}

void TcpConnection::sendInLoop(const void* message, size_t len)
{
    ssize_t nwrote = 0;         // 已经发送的字节数
    ssize_t remaining = len;    // 剩余未发送的字节数
    bool faultError = false;    // 是否发生错误

    // 之前调用过shutdown，不能发送
    if(state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing\n");
        return;
    }
    else
    {
        if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)    // 如果当前没有正在写，并且缓冲区没有数据
        {
            nwrote = ::write(channel_->fd(), message, len);     // 写入socket
            if(nwrote >= 0) // 写入成功
            {
                remaining = len - nwrote;
                if(remaining == 0 && writeCompleteCallback_)    // 写入完成
                {
                    // 发送完成，不需要EPOLLOUT，再去执行handleWrite
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));      // 写入完成回调
                }
            }
            else    // 写入失败 nwrote < 0
            {
                nwrote = 0;
                if(errno != EWOULDBLOCK)    // EWOULDBLOCK表示缓冲区满了，可以继续写, 表示有真正的错误
                {
                    LOG_ERROR("TcpConnection::sendInLoop");
                    if(errno == EPIPE || errno == ECONNRESET)    // 对端关闭了连接
                    {
                        faultError = true;
                    }
                }
            }
        }
    }

    if(!faultError && remaining > 0)    // 如果没有错误，并且还有剩余数据没有写完
    {   
        // 保存剩余数据到缓冲区，给channel注册EPOLLOUT，等待下次写
        // poller会监听到channel可写事件，然后调用handleWrite
        // 用tcpconnection::handleWrite来继续写
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen + remaining >= highWaterMark_
            && oldlen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(message) + nwrote, remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting();  // 注册channel的写事件，否则poller不会给channel通知可写事件EPOLLOUT
        }
    }
}

void TcpConnection::shutdown()
{
    // FIX ME: compare and swap 好像不需要，因为限定在同一Eventloop线程中
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIX ME: shared_from_this()?
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}


void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting())     // 如果没有正在写，则直接关闭写端
    {
      // we are not writing
      socket_->shutdownWrite();
    }
}

void TcpConnection::connectionEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());          // 防止channel_被析构, tie_是一个weakptr
    channel_->enableReading();                  // 注册channel的读事件，否则poller不会给channel通知可读事件EPOLLIN
    connectionCallback_(shared_from_this());    // 连接建立，执行回调
}

void TcpConnection::connectionDestroyed()
{
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();                     // 禁用channel的读写事件
        connectionCallback_(shared_from_this());    // 连接断开，执行回调
    }

    channel_->remove();                             // 将channel从poller中移除
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
            LOG_ERROR("TcpConnection::handleWrite\n");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection::handleWrite fd = %d is down, no more writing\n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd = %d state = %s\n", channel_->fd(), to_string(state_).c_str());
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);   // 通知连接建立
    closeCallback_(connPtr);        // 通知连接关闭
}
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;    
    }
    LOG_ERROR("TcpConnection::handleError name = [%s] - SO_ERROR = %d\n", name_.c_str(), err);
}
