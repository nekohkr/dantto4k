#include "config.h"
#include <iostream>

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

Config parseConfig(const std::string& filename)
{
     Config config;
     std::ifstream file(filename);
     if (!file.is_open()) {
         throw std::runtime_error("Unable to open file: " + filename);
     }

     std::string currentSection;
     std::string line;

     while (std::getline(file, line)) {
         line = trim(line);

         if (line.empty() || line[0] == '#' || line[0] == ';') {
             continue;
         }

         if (line.front() == '[' && line.back() == ']') {
             currentSection = line.substr(1, line.size() - 2);
             currentSection = trim(currentSection);
             continue;
         }

         if (currentSection == "bondriver") {
             size_t equalPos = line.find('=');
             if (equalPos != std::string::npos) {
                 std::string key = trim(line.substr(0, equalPos));
                 std::string value = trim(line.substr(equalPos + 1));

                 if (key == "bondriverPath") {
                     config.bondriverPath = value;
                 }
                 if (key == "dumpMmtsPath") {
                     config.dumpMmtsPath = value;
                 }
             }
         }
     }

     file.close();
     return config;
}
