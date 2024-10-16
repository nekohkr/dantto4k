#include <iostream>
#include <map>
#include "stream.h"
#include "MmtTlvDemuxer.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "plt.h"
#include "mpt.h"
#include "mhTot.h"
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
#include "adtsConverter.h"

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

unsigned char* buffer = nullptr;
AVIOContext* avio_ctx = nullptr;

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
    {
        try {
            std::string path = getConfigFilePath(hModule);
            Config config = loadConfig(path);

            demuxer.init();
            buffer = (unsigned char*)av_malloc(4096);
            avio_ctx = avio_alloc_context(buffer, 4096, 1, nullptr, nullptr, outputFilter, nullptr);

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
    if (demuxer.mapStream.size() == 0) {
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
    for (auto stream : demuxer.mapStream) {
        if (stream.second->codecType == AVMEDIA_TYPE_VIDEO ||
            stream.second->codecType == AVMEDIA_TYPE_AUDIO ||
            stream.second->codecType == AVMEDIA_TYPE_SUBTITLE) {
            AVStream* outStream = avformat_new_stream(outputFormatContext, nullptr);

            outStream->codecpar->codec_type = stream.second->codecType;
            outStream->codecpar->codec_id = stream.second->codecId;
            outStream->codecpar->codec_tag = stream.second->codecTag;
            if (outStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                outStream->codecpar->sample_rate = 48000;
                outStream->codecpar->codec_id = AV_CODEC_ID_AAC;
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

static void buffer_free_callback(void* opaque, uint8_t* data) {
    free(data);
}

void processMuxing() {
    if(stream_idx != demuxer.mapStream.size()) {
        if (initStream()) {
        }
        else {
            return;
        }
    }

    for (auto packet : demuxer.avpackets) {
        AVStream* outStream = outputFormatContext->streams[packet->stream_index];
        auto it = std::next(demuxer.mapStream.begin(), packet->stream_index);
        if (it == demuxer.mapStream.end()) {
            continue;
        }
        AVRational tb = it->second->timeBase;

        if (outStream->codecpar->codec_id == AV_CODEC_ID_AAC) {
            ADTSConverter converter;
            std::vector<uint8_t> output;
            if (!converter.convert(packet->buf->data, packet->size, output)) {
                continue;
            }

            uint8_t* data = (uint8_t*)malloc(output.size() + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(data, output.data(), output.size());
            memset(data + output.size(), 0, AV_INPUT_BUFFER_PADDING_SIZE);
            AVBufferRef* buf = av_buffer_create(data, output.size() + AV_INPUT_BUFFER_PADDING_SIZE, buffer_free_callback, NULL, 0);
            AVPacket* adtsPacket = av_packet_alloc();
            adtsPacket->buf = buf;
            adtsPacket->data = buf->data;
            adtsPacket->pts = packet->pts;
            adtsPacket->dts = packet->dts;
            adtsPacket->stream_index = packet->stream_index;
            adtsPacket->flags = packet->flags;
            adtsPacket->pos = -1;
            adtsPacket->duration = 0;
            adtsPacket->size = buf->size - AV_INPUT_BUFFER_PADDING_SIZE;

            av_packet_rescale_ts(adtsPacket, tb, outStream->time_base);
            if (av_interleaved_write_frame(outputFormatContext, adtsPacket) < 0) {
                std::cerr << "Error muxing packet." << std::endl;
                continue;
            }

            av_packet_unref(adtsPacket);
        }
        else {
            av_packet_rescale_ts(packet, tb, outStream->time_base);
            if (av_interleaved_write_frame(outputFormatContext, packet) < 0) {
                std::cerr << "Error muxing packet." << std::endl;
                continue;
            }
        }

        av_packet_unref(packet);
    }

    for (auto table : demuxer.tables) {
        switch (table->getTableId()) {
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
            handler.onMhEit(std::dynamic_pointer_cast<MhEit>(table));
            break;
        case MMT_TABLE_ID::MH_SDT:
            handler.onMhSdt(std::dynamic_pointer_cast<MhSdt>(table));
            break;
        case MMT_TABLE_ID::MPT:
            handler.onMpt(std::dynamic_pointer_cast<Mpt>(table));
            break;
        case MMT_TABLE_ID::PLT:
            handler.onPlt(std::dynamic_pointer_cast<Plt>(table));
            break;
        case MMT_TABLE_ID::MH_TOT:
            handler.onMhTot(std::dynamic_pointer_cast<MhTot>(table));
            break;
        }
    }

    for (auto table : demuxer.tlvTables) {
        switch (table->tableId) {
        case TLV_TABLE_ID::TLV_NIT:
            auto it = std::dynamic_pointer_cast<TlvNit>(table);
            handler.onTlvNit(it);
            break;
        }
    }
}