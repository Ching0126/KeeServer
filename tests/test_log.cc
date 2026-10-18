#include<Logger.h>

#include<iostream>
#include<string>

int main(){
    std::string captured;
    KeeServer::Logger::setOutput([&](const char* msg,int len){
        captured.append(msg,static_cast<size_t>(len));//输出放在captured字符串内。
    });
    LOG_INFO << "hello" << 42;
    std::cout<<captured<<std::endl;
    if(captured.find("INFO")==std::string::npos){
        std::cerr<<"Fail:missing level\n";
        return 1;
    }
    if(captured.find("hello") == std::string::npos || captured.find("42") == std::string::npos){
        std::cerr<<"Fail:stream should contain hello42\n";
        return 1;
    }
    if(captured.find("test_log.cc") == std::string::npos){
        std::cerr<<"Fail:should record source file\n";
        return 1;
    }
    if(captured.empty()||captured.back() != '\n'){
        std::cerr<<"Fail:log line should end with newline\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}