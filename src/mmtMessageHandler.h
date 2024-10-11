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

class AVFormatContext;
class MhEit;
class MhSdt;
class Plt;
class TlvNit;
class Mpt;
class MhTot;

class MmtMessageHandler {
public:
	MmtMessageHandler(AVFormatContext** outputFormatContext)
		: outputFormatContext(outputFormatContext) {
	}
	void onMhEit(uint8_t tableId, const MhEit* mhEit);
	void onMhSdt(const MhSdt* mhSdt);
	void onPlt(const Plt* plt);
	void onMpt(const Mpt* mpt);
	void onTlvNit(const TlvNit* tlvNit);
	void onMhTot(const MhTot* mhTot);

protected:
	AVFormatContext** outputFormatContext;

	std::map<uint16_t, uint16_t> mapService2Pid;
	std::vector<uint8_t>* muxedOutput;
	ts::DuckContext duck;

	std::map<uint16_t, uint8_t> mapCC;

	int eitCounter = 0;
	int sdtCounter = 0;
	int nitCounter = 0;
	int patCounter = 0;
	int pmtCounter = 0;
	int totCounter = 0;

	int tsid = -1;
};
