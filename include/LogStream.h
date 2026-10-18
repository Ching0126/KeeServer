#pragma once 

#include<cstring>
#include<string>
#include<cstdio>
namespace KeeServer{

constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000*1000;

template <int SIZE>//数值模板，size为整数。我知道这里是一个int值，但是具体是啥还没确定
class FixedBuffer{
public:
    FixedBuffer()
        :cur_(data_)
    {}

    void append(const char* buf , size_t len){//拷贝数据进数组
        if(avail()>static_cast<int>(len)){
            std::memcpy(cur_,buf,len);
           cur_ += len; 
        }
    }

    const char* data()const{return data_;}//返回数组开头
    int length()const{return static_cast<int>(cur_-data_);}//获取目前的长度
    int avail()const {return static_cast<int>(end()-cur_);}//数组还可以写多少
    void reset(){cur_ = data_;}//重新指向开头，也就是清空
private:
    const char* end() const {return data_ + SIZE;}//数组末尾最后一个，上限

    char data_[SIZE];//不同的类对象中的data长度可变
    //存字符的位置
    char* cur_;//下一个可写位置，初始时指向的是data的首地址
};

class LogStream{
public:
    using Buffer = FixedBuffer<kSmallBufferSize>;//默认构造类

    void append(const char* data, int len){//填入缓冲
        buffer_.append(data,static_cast<size_t>(len));
    }

    const Buffer& buffer() const { return buffer_; }//获取首地址

    //按类型重载
    LogStream& operator<<(char v){//char型直接插入
        buffer_.append(&v,1);
        return *this;
    }

    LogStream& operator<<(const char* str){//字符串，按照长度插入
        if(str){
            buffer_.append(str,std::strlen(str));
        }else{
            buffer_.append("(null)",6);//如果传的是空指针
        }
        return *this;
    }

    LogStream& operator<<(const std::string& str) {
        buffer_.append(str.data(), str.size());
        return *this;
    }

    LogStream& operator<<(int v){//整数
        char buf[32];
        const int n = std::snprintf(buf,sizeof(buf),"%d",v);//十进制转字符
        buffer_.append(buf,static_cast<size_t>(n));
        return *this;
    }

private:
    Buffer buffer_;//一块缓冲
};
}