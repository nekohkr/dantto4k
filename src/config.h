#pragma once
#include <string>
#include <fstream>

class Config {
public:
    std::string bondriverPath;
    std::string mmtsDumpPath;
};

std::string getConfigFilePath(void* hModule);
Config loadConfig(const std::string& filename);