#include <Buffer.h>

#include<cerrno>
#include<sys/uio.h>
#include<unistd.h>

namespace KeeServer{

ssize_t Buffer::readFd(int fd,int* saveErrno){//从某个fd把内核中的数据存进buffer,一次尽量多拿
    char extrabuf[65536];//额外缓存
    /*
    struct iovec {
    void  *iov_base;  // 这块缓冲区从哪开始
    size_t iov_len;   // 这块有多长
    };
    */
    struct iovec vec[2];
    const size_t writable = writableBytes();//尾部可用空间大小

    vec[0].iov_base = begin() + writerIndex_;//Buffer尾部空位
    vec[0].iov_len = writable;//空间大小
    vec[1].iov_base = extrabuf;//栈上64KB
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf))?2:1;//用一块缓冲区还是两块
    const ssize_t n = ::readv(fd,vec,iovcnt);//几块内存去read 
    //fd 从哪读
    //vec 每段内存的起始和长度 （说明书）
    //用数组里前几段内存
    if(n<0){
        *saveErrno = errno;
    }else if(static_cast<size_t>(n) <= writable){
        writerIndex_ +=n;
    }else{
        writerIndex_ = buffer_.size();
        append(extrabuf,n-writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd,int* saveErrno){//把buffer中的数据写到fd中
    const ssize_t n = ::write(fd,peek(),readableBytes());
    if(n<0){
        *saveErrno = errno;
    }
    return n;
}
}
