#pragma once

#include <functional>
#include <memory>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;
// 水位线：当缓冲区数据量达到一定值时，触发回调
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

// typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
// typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
// typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
// typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
// typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;
// // 水位线：当缓冲区数据量达到一定值时，触发回调
// typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;