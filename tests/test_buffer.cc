#include <Buffer.h>

#include<cstdlib>
#include<iostream>
#include<string>
#include<unistd.h>

int main(){
{
    KeeServer::Buffer buf;
    buf.append("hello",5);
    if(buf.readableBytes()!=5){
        std::cerr<<"FAIL:readableBytes after append\n";
        return 1;
    }
    if(buf.retrieveAllAsString()!="hello"){
        std::cerr<<"FAIL:retriveAllsString\n";
        return 1;
    }
    if(buf.readableBytes()!=0){
        std::cerr<<"FAILA:buffer should be empty after retrieveAll\n";
        return 1;
    }
}

{
    KeeServer::Buffer buf;
    buf.append("abcdef",6);
    buf.retrieve(2);
    if(buf.retrieveAllAsString()!="cdef"){
        std::cerr<<"FAIL:retrieve(2) should leave cdef\n";
        return 1;
    }
}
{
    KeeServer::Buffer buf;
    const std::string big(2000,'x');
    buf.append(big.data(),big.size());
    if(buf.retrieveAllAsString()!=big){
        std::cerr<<"FAIL: append should grow past initial 1024\n";
        return 1;
    }
}

{
    int fds[2];
    if(::pipe(fds)<0){
        std::abort();
    }
    if(::write(fds[1],"hi",2) != 2){
        std::cerr<<"FAIL: pipe write\n";
        return 1;
    }

    KeeServer::Buffer buf;
    int err = 0;
    const ssize_t n = buf.readFd(fds[0],&err);
    if(n!=2 || buf.retrieveAllAsString()!="hi"){
        std::cerr<<"FAIL:readFd small payload\n";
        return 1;
    }  
    const std::string big(2000,'y');
    if(::write(fds[1],big.data(),big.size()) != static_cast<ssize_t>(big.size())){
        std::cerr<<"FAIL:pipe write big\n";
        return 1;
    }

    err =0;
    const ssize_t n2 = buf.readFd(fds[0],&err);
    if(n2 != static_cast<ssize_t>(big.size()) || buf.retrieveAllAsString() != big){
        std::cerr<<"FAIL:readFd should use extrabuf when buffer is small\n";
        return 1;
    }

    ::close(fds[0]);
    ::close(fds[1]);
}
std::cout<<"OK\n";
return 0;
}