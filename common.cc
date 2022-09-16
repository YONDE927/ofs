#include "common.h"

#include <iostream>
#include <fstream>
#include <cstring>

ConfigReader::ConfigReader(){

}

ConfigReader::ConfigReader(std::string path){
    load(path);
}

void ConfigReader::load(std::string path){
    std::ifstream fs(path);
    std::string line;
    while(std::getline(fs, line)){
        char* str = strdup(line.c_str());

        std::string key(strtok(str, " "));
        config_map.try_emplace(key, std::vector<std::string>());
        for(;;){
            char* tp =  strtok(nullptr, " ");
            if(tp != nullptr){
                std::string val(tp);
                config_map.at(key).push_back(val);
            }else{
                break;
            }
        }
        free(str);
    }
}

void ConfigReader::print(){
    for(const auto& pair : config_map){
        std::cout << "[" << pair.first << "]: ";
        for(const auto& v : pair.second){
            std::cout << v << ", ";
        }
        std::cout << std::endl;
    }
}

int test_config(){
    std::string config = "ftpconfig";
    ConfigReader reader;
    reader.load(config);
    reader.print();
    return 0;
}
