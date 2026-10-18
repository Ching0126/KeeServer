#include<AsyncLogging.h>
#include<Logger.h>

#include<cstdio>
#include<fstream>
#include<glob.h>
#include<iostream>
#include<string>
#include<unistd.h>

namespace{

void unlinkLogs(const std::string& prefix){
    glob_t g{};
    const std::string pattern = prefix + ".*";
    if(::glob(pattern.c_str(),0,nullptr,&g)==0){
        for(size_t i=0;i<g.gl_pathc;++i){
            ::unlink(g.gl_pathv[i]);
        }
    }
    ::globfree(&g);
    ::unlink(prefix.c_str());
}

std::string readLogs(const std::string& prefix){
    glob_t g{};
    const std::string pattern = prefix + ".*";
    std::string all;
    if(::glob(pattern.c_str(),0,nullptr,&g)==0){
        for(size_t i=0;i<g.gl_pathc;++i){
            std::ifstream in(g.gl_pathv[i]);
            std::string line;
            while(std::getline(in,line)){
                all+=line;
                all+='\n';
            }
        }
    }
    ::globfree(&g);
    return all;
}

}

int main(){
    const std::string path = "keeserver_async.log";
    unlinkLogs(path);
    {
        KeeServer::AsyncLogging log(path,1*1024*1024,3);
        log.start();
        KeeServer::Logger::setOutput([&](const char* msg,int len){
            log.append(msg,len);
        });
        LOG_INFO<<"async-hello";//写进内存缓冲
        log.stop();
    }

    const std::string all = readLogs(path);
    unlinkLogs(path);

    if(all.find("async-hello")==std::string::npos){
        std::cerr<<"FAIL: file should contain async-hello after stop()\n";
        return 1;
    }
    if(all.find("INFO")==std::string::npos){
        std::cerr<<"FAIL: file should contain level\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}
