#pragma once
#include <string>
#include <fstream>

class Config {
public:
    std::string bondriverPath{};
    std::string mmtsDumpPath{};
    std::string smartCardReaderName{};
    bool disableADTSConversion{false};
};

Config loadConfig(const std::string& filename);

extern Config config;