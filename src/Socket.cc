#include <Socket.h>
#include<InetAddress.h>

#include<cstdlib>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<sys/socket.h>
#include<unistd.h>

namespace KeeServer{

int Socket::createNonblocking(){//创造非阻塞的fd
    int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);//地址族；套接字类型；协议
                        //IPV4      可靠字节流（TCP) 非阻塞不等待   自动关闭
    if(sockfd < 0){
        std::abort();
    }
    return sockfd;
}

Socket::Socket(int sockfd)
    :sockfd_(sockfd)
    {}

Socket::~Socket(){
    ::close(sockfd_);
}
    
void Socket::bindAddress(const InetAddress& localaddr){
    //bind(sockfd_,地址结构指针，sizeof(socksddr_in))
    if(::bind(sockfd_,
        reinterpret_cast<const sockaddr*>(localaddr.getSockAddr()),//sockaddr_in*
        sizeof(sockaddr_in))<0){//地址长度
        std::abort();
    }
}

void Socket::listen(){
    if(::listen(sockfd_,1024)<0){//等待队列最多1024个 任务转交给内核 ，表示内核成功监听
        std::abort();
    }
}

int Socket::accept(InetAddress* peeraddr){
    sockaddr_in addr{};//用来装ip host 客户端
    socklen_t len = sizeof(addr);//内存大小
    int connfd = ::accept4(sockfd_,
                            reinterpret_cast<sockaddr*>(&addr),
                            &len,
                            SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0){
        peeraddr->setSockAddr(addr);//连接成功之后 再记录对端地址
    }
    return connfd;
}

void Socket::shutdownWrite(){
    ::shutdown(sockfd_,SHUT_WR);
}

void Socket::setTcpNoDelay(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on){//打开或关掉地址重用，bind之前调用才有效
    int optval = on ?1 :0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
}

void Socket::setReusePort(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}

void Socket::setKeepAlive(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}
}

