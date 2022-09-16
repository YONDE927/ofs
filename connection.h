#pragma once

#include <netinet/in.h>
#include <string>
#include <csignal>

#include <arpa/inet.h>

void avoid_sigpipe();

enum resType{
    OK,
    ERROR
};

class SocketTask{
    public:
        virtual ~SocketTask();
        virtual const int run(int sd);
};

class Server{
    private:
        sockaddr_in addr_;
    public:
        Server(std::string ip, short port);
        ~Server();
        void run(SocketTask& task);
};

class Client{
    private:
        sockaddr_in addr_;
    public:
        Client(std::string ip, short port);
        ~Client();
        int run(SocketTask& task);
        int conn();
};
