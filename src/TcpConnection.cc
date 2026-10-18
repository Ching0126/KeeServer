#include <TcpConnection.h>
#include<EventLoop.h>
#include<Logger.h>

#include<cerrno>
#include<sys/sendfile.h>
#include<sys/socket.h>
#include<unistd.h>

namespace KeeServer{
TcpConnection::TcpConnection(EventLoop* loop,
            const std::string& nameArg,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr
):loop_(loop),
    name_(nameArg),
    state_(kConnecting),
    socket_(sockfd),
    channel_(loop,sockfd),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024){
        channel_.setReadCallback([this]{handleRead();});
        channel_.setWriteCallback([this]{handleWrite();});
        channel_.setCloseCallback([this]{handleClose();});
        channel_.setErrorCallback([this]{handleError();});
        socket_.setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    if(!channel_.isNoneEvent()){
        channel_.disableAll();
    }
    channel_.remove();
}

void TcpConnection::connectEstablished(){
    state_ = kConnected;
    channel_.enableReading();
    if(connectionCallback_){
        connectionCallback_(this);
    }
}

void TcpConnection::send(const std::string& buf){//跨线程入口
    if(state_!=kConnected){
        return ;
    }
    if(loop_->isInLoopThread()){
        sendInLoop(buf.data(), buf.size());
    }else{
        loop_->runInLoop([this,buf]{sendInLoop(buf.data(), buf.size());});
    }
}

void TcpConnection::sendInLoop(const char* data, size_t len){//能写就写，写不完就屯着，刚新入的数据写进fd
    if(state_ == kDisconnected){
        return ;
    }
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if(!channel_.isWriting() && outputBuffer_.readableBytes() == 0){//channel不在忙且缓冲区为空，可以直接写。
        nwrote = ::write(channel_.fd(), data, len);
        if(nwrote >= 0){
            remaining = len - static_cast<size_t>(nwrote);
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop([this]{
                    if(writeCompleteCallback_){
                        writeCompleteCallback_(this);
                    }
                });
            }
        }else{//写入失败
            nwrote = 0;
            if(errno != EWOULDBLOCK && errno != EAGAIN){
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }else{
                    handleClose();
                    return ;
                }
            }
        }
    }

    if(!faultError && remaining > 0){//剩余的数据
        const size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
            const size_t water = oldLen + remaining;
            loop_->queueInLoop([this,water]{
                if(highWaterMarkCallback_){
                    highWaterMarkCallback_(this,water);
                }
            });
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if(!channel_.isWriting()){
            channel_.enableWriting();
        }
    }
}

void TcpConnection::sendFile(int fileDescriptor,off_t offset,size_t count){
    if(state_ != kConnected){
        return ;
    }
    if(loop_->isInLoopThread()){
        sendFileInLoop(fileDescriptor,offset,count);
    }else{
        loop_->runInLoop([this,fileDescriptor,offset,count]{
            sendFileInLoop(fileDescriptor,offset,count);
        });
    }
}

void TcpConnection::sendFileInLoop(int fileDescriptor,off_t offset,size_t count){
    if(state_ == kDisconnected || state_ == kDisconnecting){
        return ;
    }
    size_t remaining = count;
    bool faultError = false;

    if(!channel_.isWriting() && outputBuffer_.readableBytes() == 0){
        const ssize_t bytesSent = ::sendfile(socket_.fd(),fileDescriptor,&offset,remaining);
        if(bytesSent >= 0){
            remaining -= static_cast<size_t>(bytesSent);
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop([this]{
                    if(writeCompleteCallback_){
                        writeCompleteCallback_(this);
                    }
                });
            }
        }else{
            if(errno == EPIPE || errno == ECONNRESET){
                faultError = true;
            }else if(errno != EWOULDBLOCK && errno != EAGAIN){
                handleClose();
                return ;
            }
        }
    }

    if(!faultError && remaining > 0){
        loop_->queueInLoop([this,fileDescriptor,offset,remaining]{
            sendFileInLoop(fileDescriptor,offset,remaining);
        });
    }
}

void TcpConnection::shutdown(){
    if(state_ == kConnected){
        state_ = kDisconnecting;
        loop_->runInLoop([this]{shutdownInLoop();});
    }
}

void TcpConnection::shutdownInLoop(){
    if(!channel_.isWriting()){
        socket_.shutdownWrite();
    }
}

void TcpConnection::handleRead(){
    int saveErrno = 0;
    const ssize_t n = inputBuffer_.readFd(channel_.fd(),&saveErrno);
    if(n>0){
        if(messageCallback_){
            messageCallback_(this,&inputBuffer_);
        }
    }else if(n==0){
        handleClose();
    }else if(saveErrno != EAGAIN && saveErrno != EWOULDBLOCK){
        handleClose();
    }
}

void TcpConnection::handleWrite(){//缓冲区中的旧货输出
    if(!channel_.isWriting()){
        return ;
    }
    int saveErrno = 0;
    const ssize_t n = outputBuffer_.writeFd(channel_.fd(),&saveErrno);
    if(n>0){
        outputBuffer_.retrieve(static_cast<size_t>(n));
        if(outputBuffer_.readableBytes() == 0){
            channel_.disableWriting();
            if(writeCompleteCallback_){
                loop_->queueInLoop([this]{
                    if(writeCompleteCallback_){
                        writeCompleteCallback_(this);
                    }
                });
            }
            if(state_ == kDisconnecting){
                shutdownInLoop();
            }
        }
    }else if(saveErrno != EAGAIN && saveErrno != EWOULDBLOCK){
        handleClose();
    }
}

void TcpConnection::handleClose(){
    state_ = kDisconnected;
    if(!channel_.isNoneEvent()){
        channel_.disableAll();
    }
    if(connectionCallback_){
        connectionCallback_(this);
    }
    if(closeCallback_){
        const CloseCallback cb = closeCallback_;
        closeCallback_ = {};
        cb(this);
    }
}

void TcpConnection::handleError(){
    int optval = 0;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if(::getsockopt(channel_.fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0){
        err = errno;
    }else{
        err = optval;
    }
    LOG_ERROR<<"TcpConnection::handleError name:"<<name_<<" SO_ERROR:"<<err;
}

}
