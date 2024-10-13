#include <iostream>
#include <map>
#include "stream.h"
#include "MmtTlvDemuxer.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "plt.h"
#include "mpt.h"
#include "mmtTable.h"
#include "mmtMessageHandler.h"
#include "tlvNit.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include "bonTuner.h"
#include "config.h"
#include "dantto4k.h"

AVFormatContext* outputFormatContext = nullptr;
MmtTlvDemuxer demuxer;
CBonTuner bonTuner;
std::vector<uint8_t> muxedOutput;
MmtMessageHandler handler(&outputFormatContext);

std::mutex outputMutex;
std::mutex inputMutex;

class TSInput {
public:
    TSInput() : position(0) {}

    bool read(ts::TSPacket& packet) {
        if (0 >= data.size()) {
            return false;
        }

        if (188 > data.size()) {
            return false;
        }

        std::memcpy(packet.b, data.data(), 188);
        data.erase(data.begin(), data.begin() + 188);
        return true;
    }

    std::vector<uint8_t> data;

private:
    size_t position;
};

TSInput tsInput;

//filter out SDT, PAT, PMT packets creacted by ffmpeg.
int outputFilter(void* opaque, const uint8_t* buf, int buf_size) {
    std::vector<uint8_t> buffer(buf_size);
    memcpy(buffer.data(), buf, buf_size);
    
    tsInput.data.insert(tsInput.data.end(), buffer.begin(), buffer.end());
    
    ts::TSPacket packet;
    while (tsInput.read(packet)) {
        uint16_t pid = packet.getPID();
        
        if (pid == DVB_SDT_PID && packet.getPriority() == 0) {
            continue;
        }
        if (pid == MPEG_PAT_PID && packet.getPriority() == 0) {
           continue;
        }

        //PMT
        if (pid == 0x1000 && packet.getPriority() == 0) {
            continue;
        }

        outputMutex.lock();
        muxedOutput.insert(muxedOutput.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        outputMutex.unlock();
    }
    
    return buf_size;
}

extern "C" __declspec(dllexport) IBonDriver* CreateBonDriver()
{
    return &bonTuner;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
    {
        try {
            demuxer.init();
            unsigned char* buffer = (unsigned char*)av_malloc(4096);
            AVIOContext* avio_ctx = avio_alloc_context(buffer, 4096, 1, nullptr, nullptr, outputFilter, nullptr);

            avformat_alloc_output_context2(&outputFormatContext, nullptr, "mpegts", nullptr);
            if (!outputFormatContext) {
                std::cerr << "Could not create output context." << std::endl;
                return -1;
            }

            outputFormatContext->pb = avio_ctx;
            outputFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

            Config config;
            char g_IniFilePath[_MAX_FNAME];
            GetModuleFileNameA(hModule, g_IniFilePath, MAX_PATH);

            char drive[_MAX_DRIVE];
            char dir[_MAX_DIR];
            char fname[_MAX_FNAME];
            _splitpath_s(g_IniFilePath, drive, sizeof(drive), dir, sizeof(dir), fname, sizeof(fname), NULL, NULL);
            sprintf(g_IniFilePath, "%s%s%s.ini\0", drive, dir, fname);

            config = parseConfig(g_IniFilePath);
            bonTuner.init(config);
        }
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
        }
        break;
    }
    case DLL_PROCESS_DETACH:
        break;
    }

    return true;
}

void processMuxing();
unsigned char* buffer = nullptr;
AVIOContext* avio_ctx = nullptr;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "dantto4k.exe <input.mmts> <output.ts>" << std::endl;
        return 1;
    }

    demuxer.init();

    std::string inputPath, outputPath;
    inputPath = argv[1];
    outputPath = argv[2];

    FileStream input(inputPath.c_str());
    FILE* fp = fopen(outputPath.c_str(), "wb");

    buffer = (unsigned char*)av_malloc(4096);
    avio_ctx = avio_alloc_context(buffer, 4096, 1, fp, nullptr, outputFilter, nullptr);
    
    int stream_idx = 0;
    while (!input.isEOF()) {
        int n = demuxer.processPacket(input);
        processMuxing();

        outputMutex.lock();
        fwrite(muxedOutput.data(), 1, muxedOutput.size(), fp);
        muxedOutput.clear();
        outputMutex.unlock();

        if (n == -1) {
            break;
        }
    }

    av_write_trailer(outputFormatContext);

    outputMutex.lock();
    fwrite(muxedOutput.data(), 1, muxedOutput.size(), fp);
    muxedOutput.clear();
    outputMutex.unlock();

    fclose(fp);

    return 0;
}

int stream_idx = 0;

bool initStream() {
    if (demuxer.streamMap.size() == 0) {
        return false;
    }

    if (outputFormatContext) {
        av_write_trailer(outputFormatContext);
        avformat_free_context(outputFormatContext);
    }

    avformat_alloc_output_context2(&outputFormatContext, nullptr, "mpegts", nullptr);
    if (!outputFormatContext) {
        std::cerr << "Could not create output context." << std::endl;
        return false;
    }
    outputFormatContext->pb = avio_ctx;
    outputFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

    stream_idx = 0;

    int i = 0;
    for (auto stream : demuxer.streamMap) {
        if (stream.second->codecType == AVMEDIA_TYPE_VIDEO ||
            stream.second->codecType == AVMEDIA_TYPE_AUDIO ||
            stream.second->codecType == AVMEDIA_TYPE_SUBTITLE) {
            AVStream* out_stream = avformat_new_stream(outputFormatContext, nullptr);

            out_stream->codecpar->codec_type = stream.second->codecType;
            out_stream->codecpar->codec_id = stream.second->codecId;
            out_stream->codecpar->codec_tag = stream.second->codecTag;
            if (out_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                out_stream->codecpar->codec_tag = 0;
                out_stream->codecpar->sample_rate = 48000;
            }
            stream_idx++;
        }
        i++;
    }

    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
        std::cerr << "Could not open output file." << std::endl;
        avformat_free_context(outputFormatContext);
        return false;
    }

    return true;
}

void processMuxing() {
    if(stream_idx != demuxer.streamMap.size()) {
        if (initStream()) {
        }
        else {
            return;
        }
    }

    for (auto packet : demuxer.avpackets) {
        AVStream* out_stream;
        out_stream = outputFormatContext->streams[packet->stream_index];
        AVRational tb = (*demuxer.streamMap.begin()).second->timeBase;

        av_packet_rescale_ts(packet, tb, out_stream->time_base);
        if (av_interleaved_write_frame(outputFormatContext, packet) < 0) {
            std::cerr << "Error muxing packet." << std::endl;
            continue;
        }

        av_packet_unref(packet);
    }

    for (auto table : demuxer.tables) {
        switch (table->tableId) {
        case MMT_TABLE_ID::MH_EIT:
        case 0x8C:
        case 0x8D:
        case 0x8E:
        case 0x8F:
        case 0x90:
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97:
        case 0x98:
        case 0x99:
        case 0x9A:
        case 0x9B:
            handler.onMhEit(table->tableId, (MhEit*)table);
            break;
        case MMT_TABLE_ID::MH_SDT:
            handler.onMhSdt((MhSdt*)table);
            break;
        case MMT_TABLE_ID::MPT:
            handler.onMpt((Mpt*)table);
            break;
        case MMT_TABLE_ID::PLT:
            handler.onPlt((Plt*)table);
            break;
        case MMT_TABLE_ID::MH_TOT:
            handler.onMhTot((MhTot*)table);
            break;
        }
    }

    for (auto table : demuxer.tlvTables) {
        switch (table->tableId) {
        case TLV_TABLE_ID::TLV_NIT:
            handler.onTlvNit((TlvNit*)table);
            break;
        }
    }
}