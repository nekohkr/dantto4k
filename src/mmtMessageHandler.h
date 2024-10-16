#pragma once
#include <tsduck.h>

#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
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
class MhEit;
class MhSdt;
class Plt;
class TlvNit;
class Mpt;
class MhTot;

class MmtMessageHandler {
public:
	MmtMessageHandler(struct AVFormatContext** outputFormatContext)
		: outputFormatContext(outputFormatContext) {
	}
	void onMhEit(const std::shared_ptr<MhEit>& mhEit);
	void onMhSdt(const std::shared_ptr<MhSdt>& mhSdt);
	void onPlt(const std::shared_ptr<Plt>& plt);
	void onMpt(const std::shared_ptr<Mpt>& mpt);
	void onMhTot(const std::shared_ptr<MhTot>& mhTot);
	void onTlvNit(const std::shared_ptr<TlvNit>& tlvNit);

protected:
	struct AVFormatContext** outputFormatContext;

	std::map<uint16_t, uint16_t> mapService2Pid;
	std::map<uint16_t, uint8_t> mapCC;
	int tsid = -1;

	ts::DuckContext duck;
};
