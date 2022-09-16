#pragma once
#include "connection.h"
#include "ftp-type.h"

#include <netinet/in.h>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <sys/stat.h>
#include <dirent.h>

class FtpClient{
    private:
        Client client_;
        std::mutex mtx_;
        std::thread request_sender;
    public:
        FtpClient(std::string ip, short port);
        ~FtpClient();
        int echoback_(std::string msg);
        int lock_(std::string path, ftp::lockType type);
        int getattr_(std::string path, struct stat& stbuf);
        int readdir_(std::string path, std::vector<dirent>& dirents);
        int read_(std::string path, int offset, int size,
                std::vector<char>& buffer);
        int write_(std::string path, int offset, int size,
                const char* buffer);
        int create_(std::string path);
};
