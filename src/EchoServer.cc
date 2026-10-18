#include <EchoServer.h>
#include <Buffer.h>
#include <Logger.h>
#include <TcpConnection.h>

#include <functional>

namespace KeeServer {

EchoServer::EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
    : server_(loop, addr, name)
    , cache_(kCacheCapacity)
{
    server_.setConnectionCallback(
        [this](TcpConnection* conn) { onConnection(conn); });
    server_.setMessageCallback(
        [this](TcpConnection* conn, Buffer* buf) { onMessage(conn, buf); });
    server_.setThreadInitCallback([](EventLoop*) {
        LOG_INFO << "IO thread reactor started";
    });
    server_.setThreadNum(3);
}

void EchoServer::onConnection(TcpConnection* conn) {
    if (conn->connected()) {
        LOG_INFO << "Connection UP :" << conn->name() << " "
                 << conn->peerAddress().toIpPort();
    } else {
        LOG_INFO << "Connection DOWN :" << conn->name() << " "
                 << conn->peerAddress().toIpPort();
    }
}

void EchoServer::onMessage(TcpConnection* conn, Buffer* buf) {
    const std::string msg = buf->retrieveAllAsString();
    const int key = static_cast<int>(std::hash<std::string>{}(msg));
    cache_.put(key, msg);
    LOG_INFO << "cache put key=" << key << " bytes=" << static_cast<int>(msg.size());
    std::string out;
    if (cache_.get(key, out)) {
        LOG_INFO << "cache get hit key=" << key;
        conn->send(out);
    } else {
        LOG_WARN << "cache get miss key=" << key;
    }
}

}  // namespace KeeServer
