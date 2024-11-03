#pragma once
#include "mmttlvdemuxer.h"
#include <mutex>

extern MmtTlv::MmtTlvDemuxer demuxer;
extern std::vector<uint8_t> muxedOutput;
extern std::mutex inputMutex;
extern std::mutex outputMutex;
