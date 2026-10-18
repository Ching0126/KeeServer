#include <EchoServer.h>
#include <EventLoop.h>
#include <InetAddress.h>
#include <Logger.h>
#include <AsyncLogging.h>
#include <memoryPool.h>

#include <cstdio>
#include <iostream>
#include <libgen.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

static const off_t kRollSize = 1*1024*1024;

KeeServer::AsyncLogging* g_asyncLog = nullptr;

KeeServer::AsyncLogging* getAsyncLog() {
    return g_asyncLog;
}

void asyncLog(const char* msg, int len) {
    std::fwrite(msg, 1, static_cast<size_t>(len), stdout);
    std::fflush(stdout);
    KeeServer::AsyncLogging* logging = getAsyncLog();
    if (logging) {
        logging->append(msg, len);
    }
}

struct PoolProbe {
    int value;
    explicit PoolProbe(int v)
        : value(v) {}
};

int main(int argc, char* argv[]) {
    const std::string LogDir = "logs";
    ::mkdir(LogDir.c_str(), 0755);

    std::ostringstream LogfilePath;
    LogfilePath << LogDir << "/" << ::basename(argv[0]);

    KeeServer::AsyncLogging log(LogfilePath.str(), kRollSize);
    g_asyncLog = &log;
    KeeServer::Logger::setOutput(asyncLog);
    log.start();
    LOG_INFO << "async log started, also printing to terminal, rollSize="
             << static_cast<int>(kRollSize);

    memoryPool::HashBucket::initMemoryPool();
    PoolProbe* probe = memoryPool::newElement<PoolProbe>(42);
    LOG_INFO << "memory pool newElement value=" << probe->value;
    memoryPool::deleteElement(probe);
    LOG_INFO << "memory pool deleteElement done";

    KeeServer::EventLoop loop;
    KeeServer::InetAddress addr(8080);
    KeeServer::EchoServer server(&loop, addr, "EchoServer");
    LOG_INFO << "cache capacity=" << KeeServer::EchoServer::kCacheCapacity;
    LOG_INFO << "IO thread pool size=3, main reactor listens 8080";
    server.start();

    loop.runEvery(5.0, [] {
        LOG_INFO << "timer heartbeat, multi-reactor server running";
    });

    std::cout << "================================================Start Web Server================================================" << std::endl;
    loop.loop();
    std::cout << "================================================Stop Web Server=================================================" << std::endl;

    log.stop();
}
