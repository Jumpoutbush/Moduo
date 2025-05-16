#pragma once

#include "const.h"

/**
 * @brief 
    --------------------------------------
    | 预分配空间 | 可读数据 | 可写数据 |
    --------------------------------------
    ^             ^         ^
    |             |         |
    |             |         可写数据的起始位置（writerIndex_）
    | 可读数据的起始位置（readerIndex_）
    可前置写入的字节数（KCheapPrend）
 */
class Buffer
{
public:
    static const size_t KCheapPrend = 8;        // 预分配8个字节
    static const size_t kInitialSize = 1024;    // 初始大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(KCheapPrend + initialSize), 
          readerIndex_(KCheapPrend), 
          writerIndex_(KCheapPrend) 
    {}  
    // 缓冲区中可读数据的字节数
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }
    // 缓冲区中可写数据的字节数
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    // 返回缓冲区中可前置写入的字节数
    size_t prependableBytes() const
    {
        return readerIndex_;
    }
    char* beginWrite()
    {
        return begin() + writerIndex_;
    }
    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }
    // 缓冲区可读数据起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // 读取len个字节
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }
    // 读取所有可读数据, 复位
    void retrieveAll()
    {
        readerIndex_ = KCheapPrend;
        writerIndex_ = KCheapPrend;
    }
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());   // 可读取数据长度
    }
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    
    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len);
        }
    }
    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableBytes() < len + KCheapPrend) //  If there is not enough space in the buffer to add the given length plus the constant KCheapPrend
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,  // from
                      begin() + writerIndex_,  // end
                      begin() + KCheapPrend);  // to
            readerIndex_ = KCheapPrend;
            writerIndex_ += len - readable;
        }
    }
    // [data, data + len] 添加到 writable()
    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    // 读fd数据
    ssize_t readFd(int fd, int* savedErrno);

    // 写fd数据
    ssize_t writeFd(int fd, int* savedErrno);
private:
    // Returns a pointer to the beginning of the buffer
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    char* begin()
    {
        return &*buffer_.begin();
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};