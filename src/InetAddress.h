#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

#include "const.h"

class InetAddress
{
public:
    // Constructor that initializes the InetAddress with a port and an optional IP address.
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    // Constructor that initializes the InetAddress with an existing sockaddr_in structure.
    explicit InetAddress(const sockaddr_in& addr) : addr_(addr){}
    // Returns the IP address as a string.
    std::string toIp() const;
    // Returns the IP address and port as a string in the format "IP:PORT".
    std::string toIpPort() const;
    // Returns the port number as an unsigned 16-bit integer.
    uint16_t toPort() const;
    // Returns a pointer to the internal sockaddr_in structure.
    const sockaddr_in* getSockAddr() const {return &addr_;}
    void setSockAddr(const sockaddr_in& addr) {addr_ = addr;}
private:
    sockaddr_in addr_;
};