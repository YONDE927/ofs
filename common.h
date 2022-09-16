#pragma once

#include <string>
#include <vector>
#include <map>

class ConfigReader{
    public:
        std::map<std::string, std::vector<std::string>> config_map;
        ConfigReader();
        ConfigReader(std::string path);
        void load(std::string path);
        void print();
};
