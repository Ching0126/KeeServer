#include <EventLoop.h>
#include <EventLoopThreadPool.h>

#include<future>
#include<iostream>
#include<thread>

int main(){
    {
        KeeServer::EventLoop base;
        KeeServer::EventLoopThreadPool pool(&base,"zero");
        pool.start();
        if(pool.getNextLoop() != &base){
            std::cerr<<"Fail: 0 threads should use base loop\n";
            return 1;
        }
    }
    {
        KeeServer::EventLoop base;
        KeeServer::EventLoopThreadPool pool(&base,"io");
        pool.setThreadNum(2);
        pool.start();
        
        KeeServer::EventLoop* l0 = pool.getNextLoop();
        KeeServer::EventLoop* l1 = pool.getNextLoop();
        KeeServer::EventLoop* l2 = pool.getNextLoop();
        if(l0 == &base || l1 ==&base){
            std::cerr<<"Fail: subLoop must not be the base loop\n";
            return 1;
        }
        if(l0 == l1){
            std::cerr<<"Fail: two loops should be two loops\n";
            return 1;
        }
        if(l0 != l2){
            std::cerr<<"Fail: getNextLoop should return loops in round-robin\n";
            return 1;
        }
        if(l0->isInLoopThread()){//判断是否在当前线程 l0和l1是io线程的eventloop对象，当前线程是主线程，所以不在当前线程
            std::cerr<<"Fail: caller is not the subLoop thread\n";
            return 1;
        }

        std::promise<std::thread::id> p0;
        std::promise<std::thread::id> p1;
        l0->runInLoop([&]{p0.set_value(std::this_thread::get_id());});
        l1->runInLoop([&]{p1.set_value(std::this_thread::get_id());});
        const auto id0 = p0.get_future().get();
        const auto id1 = p1.get_future().get();
        if(id0 == id1){
            std::cerr<<"Fail: loops should run on different threads\n";
            return 1;
        }
        if(id0 == std::this_thread::get_id() || id1 == std::this_thread::get_id()){
            std::cerr<<"Fail: loops  run in main threads\n";
            return 1;
        }
    }
    std::cout<<"OK\n";
    return 0;
}