#pragma once
#include "connection.h"
#include "ftp-type.h"

#include <list>
#include <map>
#include <netinet/in.h>
#include <string>
#include <mutex>

class FtpServer: public SocketTask{
    private:
        std::list<in_addr_t> host_list;
        std::mutex mtx_;
        struct lock_info{
            std::list<in_addr_t> rlock_hosts_;
            in_addr_t wlock_host_{0};
        };
        std::map<std::string, lock_info> lock_map;
    private:
        in_addr_t get_in_addr(int sd);
        bool auth_host(int sd);
        int echoback_(int sd);
        int check_lock(const ftp::lockReq& req, in_addr_t host_addr);
        int do_lock(const ftp::lockReq& req, in_addr_t host_addr);
        int check_unlock(const ftp::lockReq& req, in_addr_t host_addr);
        int do_unlock(const ftp::lockReq& req, in_addr_t host_addr);
        int lock_(int sd);
        int getattr_(int sd);
        int readdir_(int sd);
        int read_(int sd);
        int write_(int sd);
        int create_(int sd);
    public:
        FtpServer(std::list<in_addr_t> host_list_):host_list{host_list_}{}
        ~FtpServer() override;
        const int run(int socket) override;
};
