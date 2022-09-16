#include "common.h"
#include "ftp-type.h"
#include "ftpclient.h"
#include "ftpserver.h"
#include "connection.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <list>
#include <cerrno>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>

void print_errno(int errno_){
    std::cout << strerror(errno_) << std::endl;
}

void echoback_repl(FtpClient& client){
    std::string msg;
    std::cout << "msg: ";
    std::cin >> msg;
    client.echoback_(msg);
}

void getattr_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    struct stat stbuf;
    int rc = client.getattr_(path, stbuf);
    if(rc == 0){
        std::cout << stbuf.st_size << " " << stbuf.st_ctime 
            << " " << stbuf.st_mtime << std::endl;
    }else if(rc > 0){
        print_errno(rc);
    }
}

void readdir_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    std::vector<dirent> dirents;
    int rc = client.readdir_(path, dirents);
    if(rc == 0){
        for(auto& de: dirents){
            std::cout << de.d_name << std::endl;
        }
    }else if(rc > 0){
        print_errno(rc);
    }
}

int input_int(){
    int i{0};
    std::string size_s;
    std::cin >> size_s;
    try{
        i = stoi(size_s); 
    }catch(const std::exception& e){
        std::cout << "invalid input" << std::endl;
        i = -1;
    }
    return i;
}

void read_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    int offset;
    std::cout << "offset: ";
    offset = input_int();
    if(offset < 0){
        return;
    }
    int size;
    std::cout << "size: ";
    size = input_int();
    if(size < 0){
        return;
    }
    std::vector<char> buffer;
    int rc = client.read_(path, offset, size, buffer);
    if(rc == 0){
        std::cout << buffer.data() << std::endl;
    }else if(rc > 0){
        print_errno(rc);
    }
}

void write_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    int offset;
    std::cout << "offset: ";
    offset = input_int();
    if(offset < 0){
        return;
    }
    int size;
    std::cout << "size: ";
    size = input_int();
    if(size < 0){
        return;
    }
    std::string buffer;
    std::cout << "buffer: ";
    std::cin >> buffer;
    int rc = client.write_(path, offset, size, buffer.c_str());
    if(rc == 0){
        std::cout << "[write success]" << std::endl;
    }else if(rc > 0){
        print_errno(rc);
    }
}

void lock_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    std::string ltype_s;
    std::cout << "lock type(rlock,wlock,unrlock,unwlock): ";
    std::cin >> ltype_s;
    ftp::lockType ltype;
    if(ltype_s == "rlock"){
        ltype = ftp::rlock;
    }else if(ltype_s == "wlock"){
        ltype = ftp::wlock;
    }else if(ltype_s == "unrlock"){
        ltype = ftp::unrlock;
    }else if(ltype_s == "unwlock"){
        ltype = ftp::unwlock;
    }else{
        return;
    }
    int rc = client.lock_(path, ltype);
    if(rc == 0){
        std::cout << "[lock success]" << std::endl;
    }else if(rc > 0){
        print_errno(rc);
    }
}

void create_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    int rc = client.create_(path);
    if(rc == 0){
        std::cout << "[create success]" << std::endl;
    }else if(rc > 0){
        print_errno(rc);
    }
}

int repl_switch(std::string oper, FtpClient& client){
    if(oper == "echoback"){
        echoback_repl(client);
    }else if(oper == "getattr"){
        getattr_repl(client);
    }else if(oper == "readdir"){
        readdir_repl(client);
    }else if(oper == "read"){
        read_repl(client);
    }else if(oper == "write"){
        write_repl(client);
    }else if(oper == "lock"){
        lock_repl(client);
    }else if(oper == "create"){
        create_repl(client);
    }else if(oper == "exit"){
        return -1;
    }
    return 0;
}

void repl(FtpClient& client){
    for(;;){
        std::string oper;
        std::cout << ">> ";
        std::cin >> oper;
        if(repl_switch(oper, client) < 0){
            break;
        }
    }
}

int client_session(std::string config_path){
    ConfigReader config(config_path);
    std::string ip, port_s;
    short port;
    try{
        ip = config.config_map.at("ip").front();
        port_s = config.config_map.at("port").front();
        port = std::stoi(port_s);
    }catch(const std::out_of_range& e){
        std::cout << "config error" << std::endl;
        return -1;
    }
    FtpClient fclient(ip, port);
    repl(fclient);
    return 0;
}

std::list<in_addr_t> convert_hostlist(const std::vector<std::string>& ip_list){
    std::list<in_addr_t> hostlist;
    for(auto& ip : ip_list){
        hostlist.push_back(inet_addr(ip.c_str()));
    }
    return hostlist;
}

int server_session(std::string config_path){
    ConfigReader config(config_path);
    std::string ip, port_s;
    std::vector<std::string> peer_ips;
    short port{0};
    try{
        ip = config.config_map.at("ip").front();
        port_s = config.config_map.at("port").front();
        peer_ips = config.config_map.at("peer");
        port = std::stoi(port_s);
    }catch(const std::out_of_range& e){
        std::cout << "config error" << std::endl;
        return -1;
    }
    Server server(ip, port);
    FtpServer fserver(convert_hostlist(peer_ips));
    server.run(fserver);
    return 0;
}


int main(int argc, char** argv){
    if(argc < 3){
        std::cout << "ftprepl [client/server] [config]" << std::endl;
        exit(0);
    }

    std::string mode = argv[1];
    std::string config_path = argv[2];

    avoid_sigpipe();

    if(mode == "client"){
        return client_session(config_path);
    }else if(mode == "server"){
        return server_session(config_path);
    }
    return 0;
}
