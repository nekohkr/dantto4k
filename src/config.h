#pragma once
#include <string>
#include <fstream>

class Config {
public:
    std::string bondriverPath;
    std::string dumpMmtsPath;

};

Config parseConfig(const std::string& filename);