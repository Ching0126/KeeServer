#include<LogFile.h>

#include<cstdio>
#include<ctime>
#include<sys/time.h>

namespace KeeServer{

LogFile::LogFile(const std::string& basename,
                 off_t rollSize,
                 int flushInterval,
                 int checkEveryN)
    :basename_(basename)
    ,rollSize_(rollSize)
    ,flushInterval_(flushInterval)
    ,checkEveryN_(checkEveryN)
    ,count_(0)
    ,startOfPeriod_(0)
    ,lastRoll_(0)
    ,lastFlush_(0)
{
    rollFile();
}

void LogFile::append(const char* data,int len){
    std::lock_guard<std::mutex> lock(mutex_);
    appendInLock(data,len);
}

void LogFile::flush(){
    file_->flush();
}

bool LogFile::rollFile(){
    time_t now = 0;
    const std::string filename = getLogFileName(basename_,&now);
    const time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
    lastFlush_ = now;
    lastRoll_ = now;
    startOfPeriod_ = start;
    file_.reset(new FileUtil(filename));
    return true;
}

std::string LogFile::getLogFileName(const std::string& basename,time_t* now){
    std::string filename = basename;
    char timebuf[32];
    struct tm tm{};
    struct timeval tv{};
    ::gettimeofday(&tv,nullptr);
    *now = tv.tv_sec;
    ::localtime_r(now,&tm);
    std::strftime(timebuf,sizeof(timebuf),".%Y%m%d-%H%M%S",&tm);
    filename += timebuf;
    char usecbuf[16];
    std::snprintf(usecbuf,sizeof(usecbuf),".%06ld",static_cast<long>(tv.tv_usec));
    filename += usecbuf;
    filename += ".log";
    return filename;
}

void LogFile::appendInLock(const char* data,int len){
    file_->append(data,static_cast<size_t>(len));
    const time_t now = ::time(nullptr);
    ++count_;

    if(file_->writtenBytes() > rollSize_){
        rollFile();
    }else if(count_ >= checkEveryN_){
        count_ = 0;
        const time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
        if(thisPeriod != startOfPeriod_){
            rollFile();
        }
    }

    if(now - lastFlush_ > flushInterval_){
        lastFlush_ = now;
        file_->flush();
    }
}

}
