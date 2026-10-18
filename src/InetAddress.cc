#include <InetAddress.h>

#include<arpa/inet.h>
#include<cstring>
#include<cstdio>

namespace KeeServer{
//内核接收的ip:二进制数
InetAddress::InetAddress(uint16_t port,std::string ip){//往成员变量addr_中填一份完整的ipv4地址
    std::memset(&addr_,0,sizeof(addr_));//清空
    addr_.sin_family = AF_INET;//声明ipv4
    addr_.sin_port = ::htons(port);//host->network short  16为整数转化为网络字节序
    ::inet_pton(AF_INET,ip.c_str(),&addr_.sin_addr);//将ip字符串转成二进制 填进 sin_addr
}

std::string InetAddress::toIp()const{//网络序列->字符串
    char buf[64] = {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort()const{
    char buf[64] = {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));//ip
    const uint16_t port = ntohs(addr_.sin_port);//port 转化为整数
    const std::size_t end = std::strlen(buf);//获取ip末尾位置，不含\0
    std::snprintf(buf + end,sizeof(buf)-end,":%u",port);//从end后面写 给出后续剩余空间 防止溢出 格式：%u  端口号
    return buf;
}

uint16_t InetAddress::toPort()const{
    return ::ntohs(addr_.sin_port);//string->int port
}
}