#pragma once 

#include <InetAddress.h>

namespace KeeServer{
/*
    AF_INET IPV4
    SOCK_STREAM TCP
    SOCK_NONBLOCK 非阻塞
    SOCK_CLOEXEC 以后exec时关闭
*/
class Socket{//套接字
public:
    //相当于工厂函数 返回fd 非阻塞tcp-socket
    static int createNonblocking();

    //构造函数和析构函数
    explicit Socket(int sockfd);//接管一个已存在的fd  防止隐式转换
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    int fd()const{return sockfd_;}//返回fd

    void bindAddress(const InetAddress& localaddr);//socket实例绑定ip host
    void listen();//socket 监听，等待连接
    int accept(InetAddress* peeraddr);//取出一个已经连接成功的fd 返回connfd 以前的sockfd_仍然监听

    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);//开发时反复启用
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;//一个socket只允许绑定一个文件描述符 不可以更改.
};
}