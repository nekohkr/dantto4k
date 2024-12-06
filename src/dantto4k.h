#pragma once
#include "mmttlvdemuxer.h"
#include <mutex>

extern MmtTlv::MmtTlvDemuxer demuxer;
extern std::vector<uint8_t> output;

#ifdef _WIN32
LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo);
#endif