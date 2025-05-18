# aux_source_directory(. SRC_LIST)
set(SRC_LIST
    Acceptor.cc
    Buffer.cc
    Channel.cc
    CurrentThread.cc
    DefaultPoller.cc
    EPollPoller.cc
    EventLoop.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
    InetAddress.cc
    Logger.cc
    Poller.cc
    Socket.cc
    TcpConnection.cc
    TcpServer.cc
    Thread.cc
    Timestamp.cc
    )
add_library(Moduo SHARED ${SRC_LIST})

target_link_libraries(Moduo)

target_include_directories(Moduo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})