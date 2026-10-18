#include<FileUtil.h>

#include<cstdlib>

namespace KeeServer{

FileUtil::FileUtil(const std::string& FileName)
    :file_(::fopen(FileName.c_str(),"a"))//文件路径，追加内容
    ,writtenBytes_(0){
    if(!file_){
        std::abort();
    }
}

FileUtil::~FileUtil(){
    if(file_){
        ::fclose(file_);
    }
}


void FileUtil::append(const char* data,size_t len){
    ::fwrite(data,1,len,file_);//第二个参数：每个块的大小，单位字节
    writtenBytes_ += static_cast<off_t>(len);
}

void FileUtil::flush(){//file堆在用户态的数据推进内核
    ::fflush(file_);
}
}