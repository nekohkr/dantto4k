#pragma once
#include <string>
#include <fstream>

class Config {
public:
    std::string bondriverPath;
    std::string mmtsDumpPath;

};

Config parseConfig(const std::string& filename);