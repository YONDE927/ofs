#include "ftpclient.h"
#include "connection.h"
#include "ftp-type.h"

#include <cstring>
#include <iostream>
#include <memory>

FtpClient::FtpClient(std::string ip, short port):client_(ip, port){
}

FtpClient::~FtpClient(){}

int FtpClient::echoback_(std::string msg){
    if(msg.size() > MSG_LEN){
        std::cout << "msg length is too big" << std::endl;
        return -1;
    }
    ftp::echobackDatagram dgm;
    std::memcpy(&dgm.msg, msg.c_str(), msg.size() + 1);

    int sd = client_.conn();
    if(sd < 0){
        return -1;
    }

    auto rtype = ftp::echoback;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        return -1;
    }

    if(send(sd, &dgm, sizeof(dgm), 0) != sizeof(dgm)){
        return -1;
    }

    ftp::echobackDatagram dgm_r;
    if(recv(sd, &dgm_r, sizeof(dgm_r), 0) != sizeof(dgm_r)){
        return -1;
    }
    std::cout << "msg: " << dgm_r.msg << std::endl;
    return 0;
}

int FtpClient::lock_(std::string path, ftp::lockType type){
    int sd = client_.conn();
    if(sd < 0){
        return -1;
    }

    auto rtype = ftp::lock;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        return -1;
    }

    ftp::lockReq req;
    req.reqtype = ftp::lock;
    req.type = type;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        return -1;
    }

    ftp::lockRes res;
    if(recv(sd, &res, sizeof(res), 0) != sizeof(res)){
        return -1;
    }
    
    return res.errno_;
}

int FtpClient::getattr_(std::string path, struct stat& stbuf){
    int sd = client_.conn();
    if(sd < 0){
        return -1;
    }

    auto rtype = ftp::getattr;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        return -1;
    }

    ftp::getattrReq req;
    req.reqtype = ftp::getattr;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        return -1;
    }

    ftp::getattrRes res;
    if(recv(sd, &res, sizeof(res), 0) != sizeof(res)){
        return -1;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        if(recv(sd, &stbuf, sizeof(stbuf), 0) != sizeof(stbuf)){
            return -1;
        }
        return 0;
    }
}

int FtpClient::readdir_(std::string path, std::vector<dirent>& dirents){
    int sd = client_.conn();
    if(sd < 0){
        return -1;
    }

    auto rtype = ftp::readdir;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        return -1;
    }

    ftp::readdirReq req;
    req.reqtype = ftp::readdir;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        return -1;
    }

    ftp::readdirRes res;
    if(recv(sd, &res, sizeof(res), 0) != sizeof(res)){
        return -1;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        struct dirent de;
        for(int i=0; i<res.ndirent; i++){
            if(recv(sd, &de, sizeof(de), 0) != sizeof(de)){
                dirents.clear();
                return -1;
            }
            dirents.push_back(de);
        }
        return 0;
    }
}

int FtpClient::read_(std::string path, int offset, int size,
        std::vector<char>& buffer){
    int sd = client_.conn();
    if(sd < 0){
        return -1;
    }

    auto rtype = ftp::read;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        return -1;
    }

    ftp::readReq req;
    req.reqtype = ftp::read;
    req.offset = offset;
    req.size = size;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        return -1;
    }

    ftp::readRes res;
    if(recv(sd, &res, sizeof(res), 0) != sizeof(res)){
        return -1;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        buffer.resize(res.size);
        if(recv(sd, buffer.data(), res.size, 0) != res.size){
            return -1;
        }
        return 0;
    }
}

int FtpClient::write_(std::string path, int offset, int size,
        const char* buffer){
    if(size < 0){
        return -1;
    }
    int sd = client_.conn();
    if(sd < 0){
        return -1;
    }

    auto rtype = ftp::write;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        return -1;
    }

    ftp::writeReq req;
    req.reqtype = ftp::write;
    req.offset = offset;
    req.size = size;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        return -1;
    }

    ftp::writeRes res;
    if(recv(sd, &res, sizeof(res), 0) != sizeof(res)){
        return -1;
    }

    if(res.errno_ != 0){
        return res.errno_;
    }else{
        if(send(sd, buffer, size, 0) != size){
            return -1;
        }
        return 0;
    }
}

int FtpClient::create_(std::string path){
    int sd = client_.conn();
    if(sd < 0){
        return -1;
    }

    auto rtype = ftp::create;
    if(send(sd, &rtype, sizeof(rtype), 0) != sizeof(rtype)){
        return -1;
    }

    ftp::createReq req;
    req.reqtype = ftp::create;
    std::memcpy(&req.path, path.c_str(), path.size() + 1);
    if(send(sd, &req, sizeof(req), 0) != sizeof(req)){
        return -1;
    }

    ftp::createRes res;
    if(recv(sd, &res, sizeof(res), 0) != sizeof(res)){
        return -1;
    }

    return res.errno_;
}


