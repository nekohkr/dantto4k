#pragma once
#include "mmttlvdemuxer.h"
#include "mmtMessageHandler.h"
#include <mutex>

extern MmtTlvDemuxer demuxer;
extern std::vector<uint8_t> muxedOutput;
extern MmtMessageHandler handler;
extern std::mutex inputMutex;
extern std::mutex outputMutex;

void processMuxing();