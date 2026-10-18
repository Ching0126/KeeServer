#include<EventLoop.h>

#include<chrono>
#include<cstdlib>
#include<future>
#include<iostream>
#include<thread>

int main(){
    KeeServer::EventLoop loop;
    std::promise<void> done;//一次性通道的写端 promise和future成对出现
    std::thread t([&]{//线程
        loop.loop();
        done.set_value();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(80));//主线程睡80

    auto fut = done.get_future();
    if(fut.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready){//去看一眼结没结束 立刻返回 等待0秒
        std::cerr<<"FAIL: loop() return before quit()\n";
        t.join();
        return 1;
    }

    loop.quit();
    if(fut.wait_for(std::chrono::seconds(1)) != std::future_status::ready){
        std::cerr<<"FAIL:loop() did not return after quit()\n";
        std::abort();
    }

    t.join();
    std::cout<<"OK\n";
    return 0;
}
