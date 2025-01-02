#pragma once
#include <tsduck.h>
#include "demuxerHandler.h"

constexpr uint8_t STREAM_TYPE_VIDEO_MPEG1					= 0x01;
constexpr uint8_t STREAM_TYPE_VIDEO_MPEG2					= 0x02;
constexpr uint8_t STREAM_TYPE_AUDIO_MPEG1					= 0x03;
constexpr uint8_t STREAM_TYPE_AUDIO_MPEG2					= 0x04;
constexpr uint8_t STREAM_TYPE_PRIVATE_SECTION				= 0x05;
constexpr uint8_t STREAM_TYPE_PRIVATE_DATA					= 0x06;
constexpr uint8_t STREAM_TYPE_ISO_IEC_13818_6_TYPE_D		= 0x0d;
constexpr uint8_t STREAM_TYPE_AUDIO_AAC						= 0x0f;
constexpr uint8_t STREAM_TYPE_AUDIO_AAC_LATM				= 0x11;
constexpr uint8_t STREAM_TYPE_VIDEO_MPEG4					= 0x10;
constexpr uint8_t STREAM_TYPE_METADATA						= 0x15;
constexpr uint8_t STREAM_TYPE_VIDEO_H264					= 0x1b;
constexpr uint8_t STREAM_TYPE_VIDEO_HEVC					= 0x24;
constexpr uint8_t STREAM_TYPE_VIDEO_VVC						= 0x33;
constexpr uint8_t STREAM_TYPE_VIDEO_CAVS					= 0x42;
constexpr uint8_t STREAM_TYPE_VIDEO_AVS2					= 0xd2;
constexpr uint8_t STREAM_TYPE_VIDEO_AVS3					= 0xd4;
constexpr uint8_t STREAM_TYPE_VIDEO_VC1						= 0xea;
constexpr uint8_t STREAM_TYPE_VIDEO_DIRAC					= 0xd1;
constexpr uint8_t STREAM_TYPE_AUDIO_AC3						= 0x81;
constexpr uint8_t STREAM_TYPE_AUDIO_DTS						= 0x82;
constexpr uint8_t STREAM_TYPE_AUDIO_TRUEHD					= 0x83;
constexpr uint8_t STREAM_TYPE_AUDIO_EAC3					= 0x87;

constexpr uint16_t PCR_PID = 0x01FF;

namespace MmtTlv {

class Plt;
class Mpt;
class MhBit;
class MhCdt;
class MhEit;
class MhSdt;
class MhTot;
class Nit;
class MmtStream;
class MmtTlvDemuxer;
class NTPv4;

}

class RemuxerHandler : public MmtTlv::DemuxerHandler {
public:
	RemuxerHandler(MmtTlv::MmtTlvDemuxer& demuxer, std::vector<uint8_t>& output)
		: demuxer(demuxer), output(output) {
	}

	// MPU data
	void onVideoData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData) override;
	void onAudioData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData) override;
	void onSubtitleData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData) override;
	void onApplicationData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData) override;

	// MMT message
	void onMhBit(const std::shared_ptr<MmtTlv::MhBit>& mhCdt) override;
	void onEcm(const std::shared_ptr<MmtTlv::Ecm>& ecm) override {}
	void onMhCdt(const std::shared_ptr<MmtTlv::MhCdt>& mhCdt) override;
	void onMhEit(const std::shared_ptr<MmtTlv::MhEit>& mhEit) override;
	void onMhSdtActual(const std::shared_ptr<MmtTlv::MhSdt>& mhSdt) override;
	void onMhTot(const std::shared_ptr<MmtTlv::MhTot>& mhTot) override;
	void onMpt(const std::shared_ptr<MmtTlv::Mpt>& mpt) override;
	void onPlt(const std::shared_ptr<MmtTlv::Plt>& plt) override;

	// TLV message
	void onNit(const std::shared_ptr<MmtTlv::Nit>& nit) override;

	// IPv6
	void onNtp(const std::shared_ptr<MmtTlv::NTPv4>& ntp) override;

	void clear();

private:
	void writeStream(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<MmtTlv::MfuData>& mfuData, const std::vector<uint8_t>& data);
	
	std::vector<uint8_t>& output;
	MmtTlv::MmtTlvDemuxer& demuxer;

	std::map<uint16_t, uint16_t> mapService2Pid;
	std::map<uint16_t, uint8_t> mapCC;

	int tsid{-1};
	int streamCount{};

	ts::DuckContext duck;
};