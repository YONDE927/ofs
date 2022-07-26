#include "common.h"
#include "ftp-type.h"
#include "ftpclient.h"
#include "ftpserver.h"
#include "connection.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include <list>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>

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
    if(client.getattr_(path, stbuf) == 0){
        std::cout << stbuf.st_size << " " << stbuf.st_ctime 
            << " " << stbuf.st_mtime << std::endl;
    }
}

void readdir_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    std::vector<dirent> dirents;
    if(client.readdir_(path, dirents) == 0){
        for(auto& de: dirents){
            std::cout << de.d_name << std::endl;
        }
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
    std::shared_ptr<char> buffer(new char[size]);
    if(client.read_(path, offset, size, buffer.get()) == 0){
        std::cout << buffer << std::endl;
    }
}

std::shared_ptr<char> dup_shared(std::string str){
    std::shared_ptr<char> buffer(new char[str.size() + 1]);
    memcpy(buffer.get(), str.c_str(), str.size() + 1);
    return buffer;
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

    std::string str;
    std::cout << "buffer: ";
    std::cin >> str;
    std::shared_ptr<char> buffer = dup_shared(str);
    if(client.write_(path, offset, size, buffer) == 0){
        std::cout << "[write success]" << std::endl;
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
    if(client.lock_(path, ltype) == 0){
        std::cout << "[lock success]" << std::endl;
    }
}

void create_repl(FtpClient& client){
    std::string path;
    std::cout << "path: ";
    std::cin >> path;
    if(client.create_(path) == 0){
        std::cout << "[create success]" << std::endl;
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
