#pragma once
#include <tchar.h>
#include <stdio.h>
#include <windows.h>

#define log_debug(format, ...) log_debug_base(TEXT("FUNC: %s()  MSG: ") TEXT(format), TEXT(__FUNCTION__), ##__VA_ARGS__)

inline void log_debug_base(const TCHAR* format, ...)
{
    va_list args;
    SYSTEMTIME time;
    TCHAR tempBuf[1920];
    TCHAR msgBuf[2048];

    GetLocalTime(&time);

    va_start(args, format);
    _vstprintf_s(tempBuf, format, args);
    _stprintf_s(msgBuf, TEXT("[dantto4k] %02d:%02d:%02d.%03d %s\n"), time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, tempBuf);

    OutputDebugString(msgBuf);
    va_end(args);
}