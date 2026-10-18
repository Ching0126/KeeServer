#include <InetAddress.h>
#include<Socket.h>

#include<arpa/inet.h>
#include<iostream>
#include<sys/socket.h>
#include<unistd.h>

int main(){
    //服务器ip host  翻译正确
    KeeServer::InetAddress addr(8080,"127.0.0.1");
    if(addr.toIp()!="127.0.0.1" ||addr.toPort() != 8080 ||
    addr.toIpPort()!= "127.0.0.1:8080"){
        std::cerr<<"FAIL: InetAddress round-trip\n";
        return 1;
    }
    //ip绑定套接字
    KeeServer::Socket listenSock(KeeServer::Socket::createNonblocking());//构造函数
    listenSock.setReuseAddr(true);
    listenSock.bindAddress(KeeServer::InetAddress(0,"127.0.0.1"));//自动随便挑选一个空闲端口 也就是传入0即可。
    listenSock.listen();

    sockaddr_in local{};//本地地址
    socklen_t len = sizeof(local);//长度
    if(::getsockname(listenSock.fd(),reinterpret_cast<sockaddr*>(&local),&len)<0){
        std::cerr<<"FAIL:getsockname\n";
        return 1;
    }
    const uint16_t port = ::ntohs(local.sin_port);
    if(port == 0){
        std::cerr<<"FAIL: bind/listen did not assign a port\n";
        return 1;
    }

    //新的fd
    int client = ::socket(AF_INET,SOCK_STREAM,0);//新建一个套接字，系统调用
    if(client<0){
        std::cerr<<"FAIL:client socket\n";
        return 1;
    }
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = ::htons(port);
    ::inet_pton(AF_INET,"127.0.0.1",&dest.sin_addr);//string >net
    
    if(::connect(client,reinterpret_cast<sockaddr*>(&dest),sizeof(dest))<0){
        std::cerr<<"FAIL: connect\n";
        ::close(client);
        return 1;
    }
    if(::write(client,"a",1)!=1){
        std::cerr<<"FAIL :client write\n";
        return 1;
    }

    KeeServer::InetAddress peer;
    int conn = listenSock.accept(&peer);//服务器获取对端地址
    if(conn<0){
        std::cerr<<"FAIL:accept should return fd after connect\n";
        ::close(client);
        return 1;
    }
    if(peer.toIp()!="127.0.0.1"){
        std::cerr<<"FAIL:perr ip\n";
        ::close(conn);
        ::close(client);
        return 1;
    }

    char c =0;
    const ssize_t n = ::read(conn,&c,1);
    if(n!=1 || c!='a'){
        std::cerr<<"FAIL:accepted fd should receive client byte\n";
        ::close(conn);
        ::close(client);
        return 1;
    }

    ::close(conn);
    ::close(client);
    std::cout<<"OK\n";
    return 0;
}