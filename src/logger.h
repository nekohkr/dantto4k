#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include <windows.h>

inline void log_debug(const char* format, ...)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    
    va_list args;
    va_start(args, format);
    
    char argsBuffer[1920];
    vsnprintf(argsBuffer, sizeof(argsBuffer), format, args);
    
    va_end(args);

    std::ostringstream oss;
    oss << "[dantto4k] "
        << std::setw(2) << std::setfill('0') << tm.tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tm.tm_min << ":"
        << std::setw(2) << std::setfill('0') << tm.tm_sec << "."
        << std::setw(3) << std::setfill('0') << ms.count() << " "
        << argsBuffer << "\n";

    std::string logMessage = oss.str();
    OutputDebugStringA(logMessage.c_str());

    std::cerr << logMessage;
}