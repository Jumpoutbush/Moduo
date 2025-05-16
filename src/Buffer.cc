#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t Buffer::readFd(int fd, int *savedErrno)
{
    char extrabuf[65536];   // 64KB
    struct iovec vec[2];    // 一个buffer_一个extrabuf
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;    // 根据大小选buf还是栈上空间，一次最多读64KB
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0)   // errno
    {
        *savedErrno = errno;
    }
    else if(static_cast<size_t>(n) <= writable) // 读取字节n小于缓冲区writable
    {
        writerIndex_ += n;  // 可放入缓冲区，writeIndex_直接覆盖前面read部分
    }
    else    // 读取字节n大于writable
    {
        writerIndex_ = buffer_.size();  // 缓冲区读完已满，writerIndex_指向末尾
        append(extrabuf, n - writable); // extrabuf放到beginWrite, append的ensure会给buffer扩容
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *savedErrno = errno;
    }
    return n;
}
    