#pragma once 

#include<string>
#include<cstdio>
#include<sys/types.h>

namespace KeeServer{

class FileUtil{
public:
    explicit FileUtil(const std::string& fileName);
    ~FileUtil();

    FileUtil(const FileUtil&) = delete;
    FileUtil& operator=(const FileUtil&) = delete;

    void append(const char* data,size_t len);//写进文件
    void flush();//缓冲推到内核
    off_t writtenBytes()const{return writtenBytes_;}
private:
    FILE* file_;
    off_t writtenBytes_;
};
}