#include<LogFile.h>

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
}

int countLogs(const std::string& prefix){
    glob_t g{};
    const std::string pattern = prefix + ".*";
    int n = 0;
    if(::glob(pattern.c_str(),0,nullptr,&g)==0){
        n = static_cast<int>(g.gl_pathc);
    }
    ::globfree(&g);
    return n;
}

}

int main(){
    const std::string prefix = "keeserver_roll";
    unlinkLogs(prefix);

    {
        KeeServer::LogFile log(prefix,32,3,1);
        const char* chunk = "abcdefghijklmnopqrstuvwxyz1234";
        log.append(chunk,30);
        log.append(chunk,30);
        log.flush();
    }

    const int n = countLogs(prefix);
    unlinkLogs(prefix);

    if(n < 2){
        std::cerr<<"FAIL: size roll should create a second log file\n";
        return 1;
    }
    std::cout<<"OK\n";
    return 0;
}
