cmake_minimum_required(VERSION 3.10)
project(KeeServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)#头文件的搜索路径

add_library(keeserver
    src/EventLoop.cc
    src/Channel.cc
    src/Poller.cc
    src/EPollPoller.cc
    src/InetAddress.cc
    src/Socket.cc
    src/Acceptor.cc
    src/Buffer.cc
    src/TcpConnection.cc
    src/TcpServer.cc
    src/Thread.cc
    src/EventLoopThread.cc
    src/EventLoopThreadPool.cc
    src/Timestamp.cc
    src/TimerQueue.cc
    src/Logger.cc
    src/FileUtil.cc
    src/LogFile.cc
    src/AsyncLogging.cc
    src/memoryPool.cc
    src/EchoServer.cc)

target_link_libraries(keeserver pthread)

add_executable(test_eventloop
    tests/test_eventloop.cc)#.cc是测试文件
target_link_libraries(test_eventloop keeserver)

add_executable(test_channel
    tests/test_channel.cc)#.cc是测试文件
target_link_libraries(test_channel keeserver)#连接线程库

add_executable(test_pipe
    tests/test_pipe.cc)
target_link_libraries(test_pipe keeserver)

add_executable(test_socket
    tests/test_socket.cc)
target_link_libraries(test_socket keeserver)

add_executable(test_acceptor
    tests/test_acceptor.cc)
target_link_libraries(test_acceptor keeserver)

add_executable(test_buffer
    tests/test_buffer.cc)
target_link_libraries(test_buffer keeserver)

add_executable(test_connection
    tests/test_connection.cc)
target_link_libraries(test_connection keeserver)

add_executable(test_echo
    tests/test_echo.cc)
target_link_libraries(test_echo keeserver)

add_executable(test_wakeup
    tests/test_wakeup.cc)
target_link_libraries(test_wakeup keeserver)

add_executable(test_threadpool
    tests/test_threadpool.cc)
target_link_libraries(test_threadpool keeserver)

add_executable(test_send
    tests/test_send.cc)
target_link_libraries(test_send keeserver)

add_executable(test_timer tests/test_timer.cc)
target_link_libraries(test_timer keeserver)

add_executable(test_log tests/test_log.cc)
target_link_libraries(test_log keeserver)

add_executable(test_asynclog tests/test_asynclog.cc)
target_link_libraries(test_asynclog keeserver)

add_executable(test_mempool tests/test_mempool.cc)
target_link_libraries(test_mempool keeserver)

add_executable(test_cache_used tests/test_cache_used.cc)
target_link_libraries(test_cache_used keeserver)

add_executable(main src/main.cc)
target_link_libraries(main keeserver)

add_executable(test_conncb tests/test_conncb.cc)
target_link_libraries(test_conncb keeserver)

add_executable(test_logfile tests/test_logfile.cc)
target_link_libraries(test_logfile keeserver)

add_executable(test_shutdown tests/test_shutdown.cc)
target_link_libraries(test_shutdown keeserver)

add_executable(test_writecomplete tests/test_writecomplete.cc)
target_link_libraries(test_writecomplete keeserver)

add_executable(test_highwater tests/test_highwater.cc)
target_link_libraries(test_highwater keeserver)

add_executable(test_sendfile tests/test_sendfile.cc)
target_link_libraries(test_sendfile keeserver)