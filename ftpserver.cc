#include "ftpserver.h"

#include "ftp-type.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <filesystem>
#include <cerrno>

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

in_addr_t FtpServer::get_in_addr(int sd){
    sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    if(getpeername(sd, (sockaddr*)&addr, &socklen) < 0){
        return -1;
    }else{
        return addr.sin_addr.s_addr;
    }
}

bool FtpServer::auth_host(int sd){
    in_addr_t addr = get_in_addr(sd);
    if(addr < 0){
        return false;
    }else{
        for(auto host_addr : host_list){
            if(addr == host_addr){
                return true;
            }
        }
    }
    return false;
}

int FtpServer::echoback_(int sd){
    //echobackReqの受取
    ftp::echobackDatagram dg;
    int dg_size = sizeof(dg);
    if(recv(sd, &dg, dg_size, 0) != dg_size){
        return -1;
    }
    //echobackの送信
    if(send(sd, &dg, dg_size, 0) != dg_size){
        return -1;
    }
    return 0;
}

int FtpServer::check_lock(const ftp::lockReq& req, in_addr_t host_addr){
    //reqtypeの確認
    if(req.type == ftp::unrlock || req.type == ftp::unwlock){
        return EINVAL;
    }
    //まずファイルが存在するか
    if(!std::filesystem::exists(req.path)){
        return ENOENT; 
    }
    //lock_mapに存在するか
    if(lock_map.find(req.path) == lock_map.end()){
        lock_map.emplace(req.path, lock_info());
    }
    //ロックの検証
    lock_info& li = lock_map.at(req.path);
    //共通の検証
    if(li.wlock_host_ > 0){
        return ENOLCK;
    }
    if(req.type == ftp::wlock){
        //wlockの場合
        if(li.rlock_hosts_.size() > 0){
            return ENOLCK;
        }
    }
    return 0;
}

int FtpServer::do_lock(const ftp::lockReq& req, in_addr_t host_addr){
    //reqtypeの確認
    if(req.type == ftp::unrlock || req.type == ftp::unwlock){
        return EINVAL;
    }
    int rc = check_lock(req, host_addr);
    if(rc == 0){
        lock_info& li = lock_map.at(req.path);
        if(req.type == ftp::rlock){
            //rlock
            li.rlock_hosts_.push_back(host_addr);
            return 0;
        }else{
            //wlock
            li.wlock_host_ = host_addr; 
            return 0;
        }
    }else{
        return rc;
    }
}

int FtpServer::check_unlock(const ftp::lockReq& req, in_addr_t host_addr){
    //reqtypeの確認
    if(req.type == ftp::rlock || req.type == ftp::wlock){
        return EINVAL;
    }
    //lock_mapに存在するか
    if(lock_map.find(req.path) == lock_map.end()){
        return ENOENT;
    }
    //ロックの検証
    lock_info& li = lock_map.at(req.path);
    if(req.type == ftp::unrlock){
        //unrlock
        for(auto addr : li.rlock_hosts_){
            if(addr == host_addr){
                return 0;
            }
        }
        return ENOLCK;
    }else{
        //unwlock
        if(li.wlock_host_ == host_addr){
            return 0;
        }else{
            return ENOLCK;
        }
    }
}

int FtpServer::do_unlock(const ftp::lockReq& req, in_addr_t host_addr){
    //reqtypeの確認
    if(req.type == ftp::rlock || req.type == ftp::wlock){
        return EINVAL;
    }

    int rc = check_unlock(req, host_addr);
    if(rc == 0){
        lock_info& li = lock_map.at(req.path);
        if(req.type == ftp::unrlock){
            //unrlock
            li.rlock_hosts_.remove(host_addr);
            return 0;
        }else{
            //unwlock
            if(li.wlock_host_ == host_addr){
                li.wlock_host_ = 0;
                return 0;
            }else{
                return ENOLCK;
            }
        }
    }else{
        return rc;
    }
}

int FtpServer::lock_(int sd){
    //lockReqの受取
    ftp::lockReq req;
    int req_size = sizeof(req);
    if(recv(sd, &req, req_size, 0) != req_size){
        return -1;
    }
    //lockReqの検証
    int err_code{0};
    //まずファイルが存在するか
    if(!std::filesystem::exists(req.path)){
        err_code = ENOENT; 
    }
    //ホストアドレスの取得
    in_addr_t addr = get_in_addr(sd);
    if(addr < 0){
        return -1;
    }
    //ロックを試みる
    std::lock_guard<std::mutex> lock(mtx_);
    //ロック可否の確認
    if(req.type == ftp::rlock || req.type == ftp::wlock){
        err_code = check_lock(req, addr);
    }else{
        err_code = check_unlock(req, addr);
    }
    //lockResの送信
    ftp::lockRes res{err_code};
    int res_size = sizeof(res);
    if(send(sd, &res, res_size, 0) != res_size){
        return -1;
    }
    //ロックの実行
    if(err_code == 0){
        if(req.type == ftp::rlock || req.type == ftp::wlock){
            err_code = do_lock(req, addr);
        }else{
            err_code = do_unlock(req, addr);
        }
    }
    return 0;
}

int FtpServer::getattr_(int sd){
    //getattrReqの受取
    ftp::getattrReq req;
    int req_size = sizeof(req);
    if(recv(sd, &req, req_size, 0) != req_size){
        return -1;
    }
   
    //lstatの実行
    struct stat stbuf{0};
    int err_code{0};
    if(lstat(req.path, &stbuf) < 0){
        err_code = errno;
    }
    //getattrResの送信
    ftp::getattrRes res{err_code};
    int res_size = sizeof(res);
    if(send(sd, &res, res_size, 0) != res_size){
        return -1;
    }
    //statの送信
    if(err_code == 0){
        int stat_size = sizeof(stbuf);
        if(send(sd, &stbuf, stat_size, 0) != stat_size){
            return -1;
        }
    }
    return 0;
}

int FtpServer::readdir_(int sd){
    //readdirReqの受取
    ftp::readdirReq req;
    int req_size = sizeof(req);
    if(recv(sd, &req, req_size, 0) != req_size){
        return -1;
    }
    //readdirの実行
    int err_code{0};
    DIR* dirp;
    dirp = opendir(req.path);
    if(dirp == nullptr){
        err_code = errno;
    }
    std::vector<dirent> dirents;
    dirent* dep;
    for(;;){
        dep = readdir(dirp);
        if(dep == nullptr){
            break;
        }else{
            dirents.push_back(*dep);
        }
    }
    closedir(dirp);
    //readdirResの送信
    ftp::readdirRes res{err_code, static_cast<int>(dirents.size())};
    int res_size = sizeof(res);
    if(send(sd, &res, res_size, 0) != res_size){
        return -1;
    }
    //dirent配列の送信
    if(dirents.size() > 0){
        int dirents_size = dirents.size() * sizeof(dirent);
        if(send(sd, dirents.data(), dirents_size, 0) != dirents_size){
            return -1;
        }
    }
    return 0;
}

int FtpServer::read_(int sd){
    int err_code{0};
    int nread{0};
    std::shared_ptr<char> buffer;
    //readReqの受取
    ftp::readReq req;
    int req_size = sizeof(req);
    if(recv(sd, &req, req_size, 0) != req_size){
        return -1;
    }
    //read sizeの確認
    if(req.size < 0){
        err_code = EINVAL;
    }else{
        //open
        int fd = open(req.path, O_RDONLY, 0);
        if(fd < 0){
            err_code = errno;
        }else{
            //read
            buffer = std::shared_ptr<char>(new char[req.size]);
            lseek(fd, req.offset, SEEK_SET);
            nread = read(fd, buffer.get(), req.size);
            if(nread < 0){
                err_code = errno;
            }
            close(fd);
        }
    }
    //readResの送信
    ftp::readRes res{err_code, nread};
    int res_size = sizeof(res);
    if(send(sd, &res, res_size, 0) != res_size){
        return -1;
    }
    //dataの送信
    if(nread > 0){
        if(send(sd, buffer.get(), nread, 0) != nread){
            return -1;
        }
    }
    return 0;
}

int FtpServer::write_(int sd){
    int err_code{0};
    //writeReqの受取
    ftp::writeReq req;
    int req_size = sizeof(req);
    if(recv(sd, &req, req_size, 0) != req_size){
        return -1;
    }
    //write req validation
    if(req.size < 0 || req.offset < 0){
        err_code = EINVAL;
    }else{
        //openable
        int fd = open(req.path, O_WRONLY, 0);
        if(fd < 0){
            err_code = errno;
        }
        close(fd);
    }
    //writeResの送信
    ftp::writeRes res{err_code};
    int res_size = sizeof(res);
    if(send(sd, &res, res_size, 0) != res_size){
        return -1;
    }
    //エラー終了
    if(err_code != 0 || req.size == 0){
        return 0;
    }
    //dataの受信
    std::shared_ptr<char> buffer(new char[req.size]);
    if(recv(sd, buffer.get(), req.size, 0) != req.size){
        return -1;
    }
    //dataの書き込み
    int fd = open(req.path, O_WRONLY, 0);
    lseek(fd, req.offset, SEEK_SET);
    write(fd, buffer.get(), req.size);
    close(fd);
    return 0;
}

int FtpServer::create_(int sd){
    //createReqの受取
    ftp::createReq req;
    int req_size = sizeof(req);
    if(recv(sd, &req, req_size, 0) != req_size){
        return -1;
    }
    //createの実行
    int err_code{0};
    if(creat(req.path, S_IRWXU) < 0){
        err_code = errno;
    }
    //createResの送信
    ftp::createRes res{err_code};
    int res_size = sizeof(res);
    if(send(sd, &res, res_size, 0) != res_size){
        return -1;
    }
    return 0;
}

FtpServer::~FtpServer(){
    //do nothing now
};

const int FtpServer::run(int socket){
    //check ip
    if(!auth_host(socket)){
        std::cout << "invalid host" << std::endl;
        close(socket);
        return -1;
    }
    while(socket > 0){
        //recv request
        enum ftp::requestType rtype;
        int rtype_size = sizeof(rtype);
        if(recv(socket, &rtype, rtype_size, 0) != rtype_size){
            close(socket);
            return -1;
        };
        int rc{0};
        //switch request
        switch(rtype){
            case ftp::echoback:
                std::cout << "echoback" << std::endl;
                rc = echoback_(socket);
                break;
            case ftp::lock:
                std::cout << "lock" << std::endl;
                rc = lock_(socket);
                break;
            case ftp::getattr:
                std::cout << "getattr" << std::endl;
                rc = getattr_(socket);
                break;
            case ftp::readdir:
                std::cout << "readdir" << std::endl;
                rc = readdir_(socket);
                break;
            case ftp::read:
                std::cout << "read" << std::endl;
                rc = read_(socket);
                break;
            case ftp::write:
                std::cout << "write" << std::endl;
                rc = write_(socket);
                break;
            case ftp::create:
                std::cout << "create" << std::endl;
                rc = create_(socket);
                break;
            default:
                break;
        };
        if(rc < 0){
            close(socket);
            rc = 0;
        }
    }
    return 0;
};
