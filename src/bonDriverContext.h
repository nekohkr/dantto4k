#pragma once
#include <memory>
#include <fstream>
#include "bonTuner.h"
#include "mmtTlvDemuxer.h"
#include "remuxerHandler.h"

class BonDriverContext {
public:
    CBonTuner bonTuner;
    MmtTlv::MmtTlvDemuxer demuxer;
    RemuxerHandler handler{demuxer};
    std::vector<uint8_t> remuxOutput;
    std::unique_ptr<std::ofstream> mmtsDumpFs;

};

extern BonDriverContext g_bonDriverContext;