#include <EventLoop.h>
#include <InetAddress.h>
#include <TcpConnection.h>
#include <TcpServer.h>

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

int connectTo(uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = ::htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&dest), sizeof(dest)) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

bool waitUntil(const std::atomic<int>& n, int expect) {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
        if (n.load() == expect) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return n.load() == expect;
}

}  // namespace

int main() {
    const uint16_t port = 18773;
    KeeServer::EventLoop loop;
    KeeServer::TcpServer server(
        &loop, KeeServer::InetAddress(port, "127.0.0.1"), "ConnCb");

    std::atomic<int> up{0};
    std::atomic<int> down{0};
    server.setConnectionCallback([&](KeeServer::TcpConnection* conn) {
        if (conn->connected()) {
            ++up;
        } else {
            ++down;
        }
    });
    server.start();

    int client = connectTo(port);
    if (client < 0) {
        std::cerr << "FAIL: client connect\n";
        return 1;
    }

    std::promise<void> done;
    std::thread t([&] {
        loop.loop();
        done.set_value();
    });

    if (!waitUntil(up, 1)) {
        std::cerr << "FAIL: connection callback should fire on accept\n";
        loop.quit();
        t.join();
        ::close(client);
        return 1;
    }
    if (down.load() != 0) {
        std::cerr << "FAIL: down should still be 0 while client is open\n";
        loop.quit();
        t.join();
        ::close(client);
        return 1;
    }

    ::close(client);
    if (!waitUntil(down, 1)) {
        std::cerr << "FAIL: connection callback should fire on close\n";
        loop.quit();
        t.join();
        return 1;
    }

    loop.quit();
    auto fut = done.get_future();
    if (fut.wait_for(std::chrono::seconds(1)) != std::future_status::ready) {
        std::cerr << "FAIL: loop() did not return after quit\n";
        std::abort();
    }
    t.join();

    std::cout << "OK\n";
    return 0;
}
