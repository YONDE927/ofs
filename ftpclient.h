#pragma once
#include "connection.h"
#include "ftp-type.h"

#include <netinet/in.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <sys/stat.h>
#include <dirent.h>

class FtpClient{
    protected:
        Client client_;
        std::mutex mtx_;
    public:
        FtpClient(std::string ip, short port);
        virtual ~FtpClient();
        int echoback_(std::string msg);
        virtual int lock_(std::string path, ftp::lockType type);
        int getattr_(std::string path, struct stat& stbuf);
        int readdir_(std::string path, std::vector<dirent>& dirents);
        int read_(std::string path, int offset, int size,
                char* buffer);
        int write_(std::string path, int offset, int size,
                std::shared_ptr<char> buffer);
        int create_(std::string path);
};

class TryFtpClient: public FtpClient{
    private:
        std::vector<std::shared_ptr<ftp::baseReq>> unsend_reqs;
        std::condition_variable cond_;
        std::thread sending_thread;
        bool term{false};
        struct trywriteReq: public ftp::writeReq{
            std::shared_ptr<char> buffer;
        };
    private:
        int switch_req(std::shared_ptr<ftp::baseReq> req);
        void sending_task();
    public:
        TryFtpClient(std::string ip, short port);
        ~TryFtpClient();
        int elock_(std::string path, ftp::lockType type, bool do_resend);
        int ewrite_(std::string path, int offset, int size,
                std::shared_ptr<char> buffer, bool do_resend);
        int ecreate_(std::string path, bool do_resend);
};
