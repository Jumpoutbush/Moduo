#pragma once

#include "const.h"
#include "InetAddress.h"
#include "Logger.h"
#include "noncopyable.h"

class InetAddress;
class Socket : public noncopyable 
{
public:
    Socket() = delete;  
    explicit Socket(int sockfd)
        : sockfd_(sockfd){}
    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress& addr);
    void listen();
    int accept(InetAddress* peeraddr);
    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};