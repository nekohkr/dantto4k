#include <iostream>
#include <map>
#include "stream.h"
#include "MmtTlvDemuxer.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "plt.h"
#include "mpt.h"
#include "mhTot.h"
#include "mhCdt.h"
#include "mmtTableBase.h"
#include "remuxerHandler.h"
#include "nit.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include "bonTuner.h"
#include "config.h"
#include "dantto4k.h"

AVFormatContext* outputFormatContext = nullptr;
AVIOContext* avioContext = nullptr;
MmtTlv::MmtTlvDemuxer demuxer;
RemuxerHandler handler(demuxer, &outputFormatContext, &avioContext);
CBonTuner bonTuner;
std::vector<uint8_t> muxedOutput;
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

// filter out SDT, PAT, PMT packets creacted by ffmpeg
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

        // PMT
        if (pid == 0x1000 && packet.getPriority() == 0) {
            continue;
        }

        // PES
        if (pid >= 0x100 && pid <= 0x200) {
            const auto& mmtStream = demuxer.mapStreamByStreamIdx[pid - 0x100];
            if (mmtStream->componentTag == -1) {
                continue;
            }

            packet.setPID(mmtStream->getTsPid());
        }
        
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            muxedOutput.insert(muxedOutput.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
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
            std::string path = getConfigFilePath(hModule);
            Config config = loadConfig(path);

            demuxer.init();
            demuxer.setDemuxerHandler(handler);

            unsigned char* avioBuffer = (unsigned char*)av_malloc(4096);
            avioContext = avio_alloc_context(avioBuffer, 4096, 1, nullptr, nullptr, outputFilter, nullptr);

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

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "dantto4k.exe <input.mmts> <output.ts>" << std::endl;
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    demuxer.init();
    demuxer.setDemuxerHandler(handler);

    std::string inputPath, outputPath;
    inputPath = argv[1];
    outputPath = argv[2];

    MmtTlv::Common::FileStream fs(inputPath.c_str());
    std::vector<uint8_t> inputBuffer;

    FILE* fp = fopen(outputPath.c_str(), "wb");

    unsigned char* avioBuffer = (unsigned char*)av_malloc(4096);
    avioContext = avio_alloc_context(avioBuffer, 4096, 1, fp, nullptr, outputFilter, nullptr);

    while (!fs.isEof()) {
        if (inputBuffer.size() < 1024 * 1024) {
            size_t readSize = std::min(static_cast<size_t>(1024 * 1024 * 20), fs.leftBytes());
            if (readSize == 0) {
                break;
            }

            std::vector<uint8_t> buffer(readSize);
            fs.read(buffer.data(), readSize);

            inputBuffer.insert(inputBuffer.end(), buffer.begin(), buffer.end());
        }

        
		MmtTlv::Common::Stream stream(inputBuffer);
		while (!stream.isEof()) {
			int pos = stream.cur;
			int n = demuxer.processPacket(stream);

			// not valid tlv
			if (n == -2) {
				continue;
			}

			// not enough buffer for tlv payload
			if (n == -1) {
				stream.cur = pos;
				break;
			}
		}

		inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + (inputBuffer.size() - stream.leftBytes()));

        {
            std::lock_guard<std::mutex> lock(outputMutex);
            fwrite(muxedOutput.data(), 1, muxedOutput.size(), fp);
            muxedOutput.clear();
        }
    }

    av_write_trailer(outputFormatContext);
    
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        fwrite(muxedOutput.data(), 1, muxedOutput.size(), fp);
        muxedOutput.clear();
    }

    fclose(fp);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cerr << "Elapsed time: " << elapsed_seconds.count() << " seconds\n";
    return 0;
}
