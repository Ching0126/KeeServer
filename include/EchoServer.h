#pragma once

#include <ArcCache.h>
#include <TcpServer.h>

#include <string>

namespace KeeServer {

class EventLoop;
class InetAddress;
class TcpConnection;
class Buffer;

class EchoServer {
public:
    static constexpr int kCacheCapacity = 5;

    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name);

    EchoServer(const EchoServer&) = delete;
    EchoServer& operator=(const EchoServer&) = delete;

    void start() { server_.start(); }

    KeeCache::ArcCache<int, std::string>& cache() { return cache_; }

private:
    void onConnection(TcpConnection* conn);
    void onMessage(TcpConnection* conn, Buffer* buf);

    TcpServer server_;
    KeeCache::ArcCache<int, std::string> cache_;
};

}  // namespace KeeServer
