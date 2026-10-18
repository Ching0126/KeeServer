#include<EventLoop.h>

#include<chrono>
#include<iostream>
#include<thread>

int main(){
    KeeServer::EventLoop loop;
    const auto& mainId = std::this_thread::get_id();//主线程id
    std::thread::id ranOn{};//线程id数组
    int n = 0;

    std::thread t([&]{loop.loop();});
    std::this_thread::sleep_for(std::chrono::milliseconds(20));//确保t已经启动，正在等待。

    const auto t0 = std::chrono::steady_clock::now();//记录当下时间
    loop.runInLoop([&]{
        ranOn = std::this_thread::get_id();
        ++n;
        loop.quit();
    });
    t.join();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

    if(n != 1){
        std::cerr<<"Fail: functor should run once\n";
        return 1;
    }

    if(ranOn == mainId || ranOn == std::thread::id{}){
        std::cerr<<"Fail: functor should run on loop thread\n";
        return 1;
    }

    if(ms > 500){
        std::cerr<<"Fail: wakeup should not wait for 10 poll timeout\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}