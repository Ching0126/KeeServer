#include<EventLoop.h>

#include<chrono>
#include<iostream>
#include<thread>

int main(){
{
    KeeServer::EventLoop loop;
    std::string order;
    const auto t0 = std::chrono::steady_clock::now();
    loop.runAfter(0.08,[&]{order += 'B';});
    loop.runAfter(0.03,[&]{order += 'A';});
    loop.runAfter(0.12,[&]{loop.quit();});

    loop.loop();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    if(order != "AB"){
        std::cerr<<"Fail: earlier timer must run first,got"<<order<<std::endl;
        return 1;
    }
    if(ms < 80){
        std::cerr<<"Fail: timers fired too soon\n";
    }
    if(ms > 1000){
        std::cerr<<"Fail : timerfd should wake loop before 10s poll\n";
        return 1;
    }
}
{
    KeeServer::EventLoop loop;
    int n = 0;
    loop.runEvery(0.04,[&]{
        ++n;
        if(n>=3){
            loop.quit();
        }
    });
    loop.loop();
    if(n!=3){
        std::cerr<<"Fail: runEvery should fire 3 times , got"<<n<<std::endl;
        return 1;
    }
}
    std::cout<<"ok\n";
    return 0;
}