#pragma once

#include<LogStream.h>

#include<functional>

namespace KeeServer{

class Logger{
public:
    enum LogLevel {DEBUG , INFO , WARN , ERROR};//四个等级

    using OutputFunc = std::function<void(const char* msg , int len)>;//输出函数

    Logger(const char* file , int line , LogLevel level);
    ~Logger();

    //返回LogStream的引用
    LogStream& stream(){return stream_;}
    
    //静态输出函数
    static void setOutput(OutputFunc out);
private:
    LogStream stream_;//这一行的正文缓冲
    LogLevel level_;//等级
    const char* file_;//源文件路径
    int line_;//传来的行号。
};
}
//前两个参数为预定义宏，这行文本的文件名 以及行号
#define LOG_DEBUG KeeServer::Logger(__FILE__, __LINE__, KeeServer::Logger::DEBUG).stream()
#define LOG_INFO  KeeServer::Logger(__FILE__, __LINE__, KeeServer::Logger::INFO).stream()
#define LOG_WARN  KeeServer::Logger(__FILE__, __LINE__, KeeServer::Logger::WARN).stream()
#define LOG_ERROR KeeServer::Logger(__FILE__, __LINE__, KeeServer::Logger::ERROR).stream()