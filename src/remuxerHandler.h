#pragma once
#include <tsduck.h>
#include "demuxerHandler.h"

#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_ISO_IEC_13818_6_TYPE_D       0x0d
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_AUDIO_AAC_LATM  0x11
#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_METADATA        0x15
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_HEVC      0x24
#define STREAM_TYPE_VIDEO_VVC       0x33
#define STREAM_TYPE_VIDEO_CAVS      0x42
#define STREAM_TYPE_VIDEO_AVS2      0xd2
#define STREAM_TYPE_VIDEO_AVS3      0xd4
#define STREAM_TYPE_VIDEO_VC1       0xea
#define STREAM_TYPE_VIDEO_DIRAC     0xd1

#define STREAM_TYPE_AUDIO_AC3       0x81
#define STREAM_TYPE_AUDIO_DTS       0x82
#define STREAM_TYPE_AUDIO_TRUEHD    0x83
#define STREAM_TYPE_AUDIO_EAC3      0x87

struct AVFormatContext;


namespace MmtTlv {

class MhEit;
class MhSdt;
class Plt;
class Mpt;
class MhTot;
class MhCdt;
class Nit;
class MpuStream;
class MmtTlvDemuxer;

}

class RemuxerHandler : public MmtTlv::DemuxerHandler {
public:
	RemuxerHandler(MmtTlv::MmtTlvDemuxer& demuxer, struct AVFormatContext** outputFormatContext, struct AVIOContext** avioContext)
		: demuxer(demuxer), outputFormatContext(outputFormatContext), avioContext(avioContext) {
	}

	// MPU data
	void onVideoData(const std::shared_ptr<MmtTlv::MpuStream> mpuStream, const std::shared_ptr<struct MmtTlv::MpuData>& mpuData) override;
	void onAudioData(const std::shared_ptr<MmtTlv::MpuStream> mpuStream, const std::shared_ptr<struct MmtTlv::MpuData>& mpuData) override;
	void onSubtitleData(const std::shared_ptr<MmtTlv::MpuStream> mpuStream, const std::shared_ptr<struct MmtTlv::MpuData>& mpuData) override;

	// MMT message
	void onEcm(const std::shared_ptr<MmtTlv::Ecm>& ecm) override {}
	void onMhCdt(const std::shared_ptr<MmtTlv::MhCdt>& mhCdt) override;
	void onMhEit(const std::shared_ptr<MmtTlv::MhEit>& mhEit) override;
	void onMhSdt(const std::shared_ptr<MmtTlv::MhSdt>& mhSdt) override;
	void onMhTot(const std::shared_ptr<MmtTlv::MhTot>& mhTot) override;
	void onMpt(const std::shared_ptr<MmtTlv::Mpt>& mpt) override;
	void onPlt(const std::shared_ptr<MmtTlv::Plt>& plt) override;

	// TLV message
	void onNit(const std::shared_ptr<MmtTlv::Nit>& nit) override;

private:
	void writeStream(const std::shared_ptr<MmtTlv::MpuStream> mpuStream, const std::shared_ptr<MmtTlv::MpuData>& mpuData, std::vector<uint8_t> data);
	void initStreams();

	struct AVFormatContext** outputFormatContext;
	struct AVIOContext** avioContext;
	MmtTlv::MmtTlvDemuxer& demuxer;

	std::map<uint16_t, uint16_t> mapService2Pid;
	std::map<uint16_t, uint8_t> mapCC;
	int tsid = -1;
	int streamCount = 0;

	ts::DuckContext duck;
};