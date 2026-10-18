#include <EchoServer.h>
#include <EventLoop.h>
#include <InetAddress.h>

#include <arpa/inet.h>
#include <chrono>
#include <fcntl.h>
#include <functional>
#include <future>
#include <iostream>
#include <string>
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

}  // namespace

int main() {
    const uint16_t port = 18772;
    KeeServer::EventLoop loop;
    KeeServer::EchoServer server(
        &loop, KeeServer::InetAddress(port, "127.0.0.1"), "Echo");

    if (KeeServer::EchoServer::kCacheCapacity != 5) {
        std::cerr << "FAIL: capacity should match original LFU CAPACITY=5\n";
        return 1;
    }

    // Same usage as original KLfuCache<int, std::string>(CAPACITY): put/get.
    server.cache().put(1, "from-project");
    std::string value;
    if (!server.cache().get(1, value) || value != "from-project") {
        std::cerr << "FAIL: EchoServer cache put/get (same API as original LFU)\n";
        return 1;
    }
    if (server.cache().get(2, value)) {
        std::cerr << "FAIL: unused key should miss on the project's cache\n";
        return 1;
    }

    // Prove ARC is on the EchoServer request path, not a standalone object.
    server.start();
    int client = connectTo(port);
    if (client < 0) {
        std::cerr << "FAIL: client connect\n";
        return 1;
    }
    const std::string payload = "arc-used-in-keeserver";
    if (::write(client, payload.data(), static_cast<ssize_t>(payload.size())) !=//把z字符串写进套接字
        static_cast<ssize_t>(payload.size())) {
        std::cerr << "FAIL: client write\n";
        ::close(client);
        return 1;
    }

    std::promise<void> done;
    std::thread t([&] {
        loop.loop();
        done.set_value();
    });

    ::fcntl(client, F_SETFL, O_NONBLOCK);
    std::string got;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
        char buf[64] = {};
        const ssize_t n = ::read(client, buf, sizeof(buf));
        if (n > 0) {
            got.append(buf, static_cast<size_t>(n));
            if (got == payload) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    loop.quit();
    auto fut = done.get_future();
    if (fut.wait_for(std::chrono::seconds(1)) != std::future_status::ready) {
        std::cerr << "FAIL: loop() did not return after quit\n";
        std::abort();
    }
    t.join();
    ::close(client);

    if (got != payload) {
        std::cerr << "FAIL: echo path should reply via ArcCache get\n";
        return 1;
    }

    const int key = static_cast<int>(std::hash<std::string>{}(payload));
    value.clear();
    if (!server.cache().get(key, value) || value != payload) {
        std::cerr << "FAIL: EchoServer onMessage did not put payload into ArcCache\n";
        return 1;
    }

    std::cout << "OK\n";
    return 0;
}
