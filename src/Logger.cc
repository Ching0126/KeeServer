#include <Logger.h>

#include<cstdio>

namespace KeeServer{

namespace{
Logger::OutputFunc g_output = [](const char* msg, int len){
    std::fwrite(msg,1,static_cast<size_t>(len),stdout);//拼接到标准输出
};

const char* levelName(Logger::LogLevel level){
    switch(level){
        case Logger::DEBUG: return "DEBUG";
        case Logger::INFO: return "INFO";
        case Logger::WARN: return "WARN";
        case Logger::ERROR: return "ERROR";
    }
    return "UNKN";
}
}

Logger::Logger(const char* file,int line,LogLevel level)
    :level_(level)
    ,file_(file)
    ,line_(line)    
{stream_<<levelName(level)<<' ';}

Logger::~Logger(){
    stream_<<" - "<<file_<<':'<<line_<<'\n';
    const LogStream::Buffer& buf = stream_.buffer();
    g_output(buf.data(),buf.length());//输出全部内容
}

void Logger::setOutput(OutputFunc out){//换输出出口函数
    g_output = std::move(out);
}

}