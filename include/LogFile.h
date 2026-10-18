#pragma once

#include<FileUtil.h>

#include<memory>
#include<mutex>
#include<string>
#include<sys/types.h>

namespace KeeServer{

class LogFile{
public:
    LogFile(const std::string& basename,
            off_t rollSize,
            int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile() = default;

    LogFile(const LogFile&) = delete;
    LogFile& operator=(const LogFile&) = delete;

    void append(const char* data,int len);
    void flush();
    bool rollFile();

private:
    void appendInLock(const char* data,int len);
    static std::string getLogFileName(const std::string& basename,time_t* now);

    const std::string basename_;
    const off_t rollSize_;
    const int flushInterval_;
    const int checkEveryN_;
    int count_;
    std::mutex mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<FileUtil> file_;
    static const int kRollPerSeconds_ = 60*60*24;
};

}
