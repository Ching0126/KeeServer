#pragma once

#include<algorithm>
#include<cstddef>
#include<string>
#include<vector>

namespace KeeServer{

class Buffer{
public:
    //两个常量
    static const size_t kCheapPrepend = 8;//开头预留8个字节，方便以后在数据前塞包头，无需整块后移
    static const size_t kInitialSize = 1024;//初始可写区域：1KB 总长度：8+1024

    explicit Buffer(size_t initialSize = kInitialSize)//构造函数
        : buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
        {}
    
    size_t readableBytes() const{return writerIndex_-readerIndex_;}//还剩多少字节没用被取走
    size_t writableBytes() const{return buffer_.size() - writerIndex_;}//尾部的可写空闲空间大小
    size_t prependableBytes() const{return readerIndex_;}//读指针前空间

    const char* peek()const{return begin()+readerIndex_;}//看数据 返回可读区的起始地址，只看不取

    void retrieve(size_t len){//认为长度为len的字符已经处理
        if(len<readableBytes()){//不能取超过已写区域的。
            readerIndex_ += len;
        }else{
            retrieveAll();
        }
    }

    void retrieveAll(){//缓存区清空，也就是读写指针都回到8
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString(){return retrieveAsString(readableBytes());}

    std::string retrieveAsString(size_t len){//从可读位置挑len个字节，转成string 
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }

    void append(const char* data,size_t len){
        ensureWritableBytes(len);//确保足够的可写空间
        std::copy(data,data+len,beginWrite());//写入
        writerIndex_+=len;//更新
    }

    ssize_t readFd(int fd,int* saveErrno);
    ssize_t writeFd(int fd,int* saveErrno);

private:
    char* begin(){return &*buffer_.begin();}//可写
    const char* begin()const{return &*buffer_.begin();}//只读
    
    char* beginWrite(){return begin()+writerIndex_;}//地址返回转到可写区域

    void ensureWritableBytes(size_t len){
        if(writableBytes()<len){//可写区域小，造空间
            makeSpace(len);
        }
    }

    void makeSpace(size_t len){
        if(writableBytes()+prependableBytes()<len + kCheapPrepend){//读指针之前的内存+尾部空位<8+len
            buffer_.resize(writerIndex_ + len);
        }else{//把中间元素左移 使右边空位变大
            const size_t readable = readableBytes();
            std::copy(begin()+readerIndex_,
                        begin()+writerIndex_,
                        begin()+kCheapPrepend);
            //std::copy( 从哪开始,  到哪结束(不含),  拷到哪 );
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
        }
    }
    std::vector<char> buffer_;//真正的内存
    size_t readerIndex_;//刻度数据从哪开始写
    size_t writerIndex_;//新数据写在哪
};
}