#pragma once
#include <tchar.h>
#include <windows.h>

inline void log_debug(const TCHAR* format, ...)
{
	va_list args;
	SYSTEMTIME time;
	TCHAR argsBuffer[1920];
	TCHAR buffer[2048];
	GetLocalTime(&time);
	va_start(args, format);
	_vstprintf_s(argsBuffer, format, args);
	_stprintf_s(buffer, TEXT("[dantto4k] %02d:%02d:%02d.%03d %s\n"), time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, argsBuffer);
	OutputDebugString(buffer);
	va_end(args);
}
