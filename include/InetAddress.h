#pragma once

#include <netinet/in.h>
#include<string>

namespace KeeServer{
class InetAddress{
public:
    //两个构造函数
    explicit InetAddress(uint16_t port = 0,std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in& addr)
        :addr_(addr)
        {}

    //IP port的翻译函数
    std::string toIp()const;
    std::string toIpPort()const;
    uint16_t toPort()const;

    //给socket用的接口
    const sockaddr_in* getSockAddr() const {return &addr_;}//给bind /connect当内核参数
    void setSockAddr(const sockaddr_in& addr){addr_ = addr;}//aceeept得到对端地址后使用
private:
    sockaddr_in addr_{};
    //结构体：sin_family(IPV4 AF_INET) sin_port(端口) sin_addr(IP)
};
}