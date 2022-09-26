#include "ftpclient.h"
#include "connection.h"
#include "ftp-type.h"

#include <asm-generic/errno.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <sys/socket.h>

FtpClient::FtpClient(std::string ip, short port):client_(ip, port){
}

FtpClient::~FtpClient(){}

int FtpClient::echoback_(std::string msg){
    std::lock_guard<std::mutex> lock(mtx_);
    if(msg.size() > MSG_LEN){
        std::cout << "msg length is too big" << std::endl;
        client_.close_socket();
        return ENETDOWN;
    }
    ftp::echobackDatagram dgm;
    std::memcpy(&dgm.msg, msg.c_str(), msg.size() + 1);

    int sd = client_.conn();
    if(sd < 0){
        client_.close_socket();
        return ENETDOWN;
    }

    auto rtype = ftp::echoback;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        client_.close_socket();
        return ENETDOWN;
    }

    if(send(sd, &dgm, sizeof(dgm), 0) != sizeof(dgm)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::echobackDatagram dgm_r;
    if(recv(sd, &dgm_r, sizeof(dgm_r), MSG_WAITALL) != sizeof(dgm_r)){
        client_.close_socket();
        return ENETDOWN;
    }
    std::cout << "msg: " << dgm_r.msg << std::endl;
    return 0;
}

int FtpClient::lock_(std::string path, ftp::lockType type){
    std::lock_guard<std::mutex> lock(mtx_);
    int sd = client_.conn();
    if(sd < 0){
        client_.close_socket();
        return ENETDOWN;
    }

    auto rtype = ftp::lock;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::lockReq req;
    req.reqtype = ftp::lock;
    req.type = type;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::lockRes res;
    if(recv(sd, &res, sizeof(res), MSG_WAITALL) != sizeof(res)){
        client_.close_socket();
        return ENETDOWN;
    }
    
    return res.errno_;
}

int FtpClient::getattr_(std::string path, struct stat& stbuf){
    int sd = client_.conn();
    if(sd < 0){
        client_.close_socket();
        std::cout << "conn socket" << std::endl;
        return ENETDOWN;
    }

    auto rtype = ftp::getattr;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        client_.close_socket();
        std::cout << "send reqtype" << std::endl;
        return ENETDOWN;
    }

    ftp::getattrReq req;
    req.reqtype = ftp::getattr;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        client_.close_socket();
        std::cout << "send req" << std::endl;
        return ENETDOWN;
    }

    ftp::getattrRes res;
    if(recv(sd, &res, sizeof(res), MSG_WAITALL) != sizeof(res)){
        client_.close_socket();
        std::cout << "recv res" << std::endl;
        return ENETDOWN;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        if(recv(sd, &stbuf, sizeof(stbuf), MSG_WAITALL) != sizeof(stbuf)){
            client_.close_socket();
            std::cout << "recv stbuf" << std::endl;
            return ENETDOWN;
        }
        return 0;
    }
}

int FtpClient::readdir_(std::string path, std::vector<dirent>& dirents){
    std::lock_guard<std::mutex> lock(mtx_);
    int sd = client_.conn();
    if(sd < 0){
        client_.close_socket();
        std::cout << "conn socket" << std::endl;
        return ENETDOWN;
    }

    auto rtype = ftp::readdir;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        client_.close_socket();
        std::cout << "send req type" << std::endl;
        return ENETDOWN;
    }

    ftp::readdirReq req;
    req.reqtype = ftp::readdir;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        client_.close_socket();
        std::cout << "send req" << std::endl;
        return ENETDOWN;
    }

    ftp::readdirRes res;
    if(recv(sd, &res, sizeof(res), MSG_WAITALL) != sizeof(res)){
        client_.close_socket();
        std::cout << "recv res" << std::endl;
        return ENETDOWN;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        struct dirent de;
        for(int i=0; i<res.ndirent; i++){
            if(recv(sd, &de, sizeof(de), MSG_WAITALL) != sizeof(de)){
                dirents.clear();
                client_.close_socket();
                std::cout << "recv dirent type" << std::endl;
                return ENETDOWN;
            }
            dirents.push_back(de);
        }
        return 0;
    }
}

int FtpClient::read_(std::string path, int offset, int size, char* buffer){
    std::lock_guard<std::mutex> lock(mtx_);
    int sd = client_.conn();
    if(sd < 0){
        client_.close_socket();
        return ENETDOWN;
    }

    auto rtype = ftp::read;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::readReq req;
    req.reqtype = ftp::read;
    req.offset = offset;
    req.size = size;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::readRes res;
    if(recv(sd, &res, sizeof(res), MSG_WAITALL) != sizeof(res)){
        client_.close_socket();
        return ENETDOWN;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        if(recv(sd, buffer, res.size, MSG_WAITALL) != res.size){
            client_.close_socket();
            return ENETDOWN;
        }
        return 0;
    }
}

int FtpClient::write_(std::string path, int offset, int size,
        std::shared_ptr<char> buffer){
    std::lock_guard<std::mutex> lock(mtx_);
    if(size < 0){
        client_.close_socket();
        return ENETDOWN;
    }
    int sd = client_.conn();
    if(sd < 0){
        client_.close_socket();
        return ENETDOWN;
    }

    auto rtype = ftp::write;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::writeReq req;
    req.reqtype = ftp::write;
    req.offset = offset;
    req.size = size;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::writeRes res;
    if(recv(sd, &res, sizeof(res), MSG_WAITALL) != sizeof(res)){
        client_.close_socket();
        return ENETDOWN;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        if(send(sd, buffer.get(), size, 0) != size){
            client_.close_socket();
            return ENETDOWN;
        }
        return 0;
    }
}

int FtpClient::create_(std::string path){
    std::lock_guard<std::mutex> lock(mtx_);
    int sd = client_.conn();
    if(sd < 0){
        client_.close_socket();
        return ENETDOWN;
    }

    auto rtype = ftp::create;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::createReq req;
    req.reqtype = ftp::create;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        client_.close_socket();
        return ENETDOWN;
    }

    ftp::createRes res;
    if(recv(sd, &res, sizeof(res), MSG_WAITALL) != sizeof(res)){
        client_.close_socket();
        return ENETDOWN;
    }

    return res.errno_;
}

//TryFtpClient

int TryFtpClient::switch_req(std::shared_ptr<ftp::baseReq> req){
    if(req == nullptr){
        return -1;
    }
    switch(req->reqtype){
        case ftp::lock:
        {
            auto lreq = static_pointer_cast<ftp::lockReq>(req);
            return elock_(lreq->path, lreq->type, false);
        }
        case ftp::write:
        {
            auto wreq = static_pointer_cast<trywriteReq>(req);
            return ewrite_(wreq->path, wreq->offset, wreq->size,
                    wreq->buffer, false);
        }
        case ftp::create:
        {
            auto creq = static_pointer_cast<ftp::createReq>(req);
            return ecreate_(req->path, false);
        }
        default:
        {
            return 0;
        }
    }
}

void TryFtpClient::sending_task(){
    int sleep_interval{0};
    const int max_interval = 5;
    for(;;){
        if(term){
            break;
        }
        std::unique_lock<std::mutex> lock(mtx_);
        cond_.wait(lock, [this](){return unsend_reqs.size() > 0;});
        lock.unlock();
        if(unsend_reqs.begin() != unsend_reqs.end()){
            auto req_ptr = unsend_reqs.front();
            int rc = switch_req(req_ptr);
            if(rc != ENETDOWN){
                unsend_reqs.erase(unsend_reqs.begin());
                sleep_interval = 0;
            }else{
                sleep(sleep_interval);
                if(sleep_interval < max_interval){
                    sleep_interval += 1;
                }
            }
        }
    }
}

TryFtpClient::TryFtpClient(std::string ip, short port): FtpClient(ip, port){
    sending_thread = std::thread(&TryFtpClient::sending_task, this); 
}

TryFtpClient::~TryFtpClient(){
    std::lock_guard<std::mutex> lock(mtx_);
    term = true;
    cond_.notify_one();
    sending_thread.join();
}

int TryFtpClient::elock_(std::string path, ftp::lockType type
        , bool do_resend){
    int rc = lock_(path, type);
    if(rc == ENETDOWN){
        if(do_resend){
            std::lock_guard<std::mutex> lock(mtx_);
            std::shared_ptr<ftp::lockReq> preq(new ftp::lockReq);
            preq->reqtype = ftp::lock;
            preq->type = type;
            std::memcpy(&preq->path, path.c_str(), path.size() + 1);
            unsend_reqs.push_back(preq); 
        }
    }
    return rc;
}


int TryFtpClient::ewrite_(std::string path, int offset, int size,
        std::shared_ptr<char> buffer, bool do_resend){
    int rc = write_(path, offset, size, buffer);
    if(rc == ENETDOWN){
        if(do_resend){
            std::shared_ptr<trywriteReq> preq(new trywriteReq);
            preq->reqtype = ftp::write;
            preq->offset = offset;
            preq->size = size;
            std::memcpy(&preq->path, path.c_str(), path.size() + 1);
            unsend_reqs.push_back(preq); 
        }
    }
    return rc;
}

int TryFtpClient::ecreate_(std::string path, bool do_resend){
    int rc = create_(path);
    if(rc == ENETDOWN){
        if(do_resend){
            std::shared_ptr<ftp::createReq> preq(new ftp::createReq);
            preq->reqtype = ftp::create;
            std::memcpy(&preq->path, path.c_str(), path.size() + 1);
            unsend_reqs.push_back(preq); 
        }
        return -1;
    }
    return rc;
}

const char* requestType_str[] = {"echoback", "lock", "getattr", "readdir",
    "read", "write", "create"};

void TryFtpClient::print_unsend_reqs(){
    for(const auto& req : unsend_reqs){
        std::cout << "type: " << requestType_str[req->reqtype];
        std::cout << " path: " << req->path << std::endl;
    }
}
