#pragma once
#include "demuxerHandler.h"
#include "b24SubtitleConvertor.h"
#include <tsduck.h>
#include <unordered_map>
#include <functional>

namespace StreamType {

constexpr uint8_t VIDEO_MPEG1 = 0x01;
constexpr uint8_t VIDEO_MPEG2 = 0x02;
constexpr uint8_t AUDIO_MPEG1 = 0x03;
constexpr uint8_t AUDIO_MPEG2 = 0x04;
constexpr uint8_t PRIVATE_SECTION = 0x05;
constexpr uint8_t PRIVATE_DATA = 0x06;
constexpr uint8_t ISO_IEC_13818_6_TYPE_D = 0x0d;
constexpr uint8_t AUDIO_AAC = 0x0f;
constexpr uint8_t AUDIO_AAC_LATM = 0x11;
constexpr uint8_t VIDEO_MPEG4 = 0x10;
constexpr uint8_t METADATA = 0x15;
constexpr uint8_t VIDEO_H264 = 0x1b;
constexpr uint8_t VIDEO_HEVC = 0x24;

}

namespace MmtTlv {

class Plt;
class Mpt;
class MhBit;
class MhAit;
class MhCdt;
class MhEit;
class MhSdt;
class MhTot;
class Nit;
class MmtStream;
class MmtTlvDemuxer;
class NTPv4;

}

constexpr uint16_t PCR_PID = 0x01FF;

class RemuxerHandler : public MmtTlv::DemuxerHandler {
public:
	RemuxerHandler(MmtTlv::MmtTlvDemuxer& demuxer)
		: demuxer(demuxer) {
	}

	// MPU data
	void onVideoData(const std::shared_ptr<MmtTlv::MmtStream>& mmtStream, const std::shared_ptr<MmtTlv::MpuData>& mfuData) override;
	void onAudioData(const std::shared_ptr<MmtTlv::MmtStream>& mmtStream, const std::shared_ptr<MmtTlv::MpuData>& mfuData) override;
	void onSubtitleData(const std::shared_ptr<MmtTlv::MmtStream>& mmtStream, const std::shared_ptr<MmtTlv::MpuData>& mfuData) override;
	void onApplicationData(const std::shared_ptr<MmtTlv::MmtStream>& mmtStream, const std::shared_ptr<MmtTlv::MpuData>& mfuData) override;

	void onPacketDrop(uint16_t packetId, const std::shared_ptr<MmtTlv::MmtStream>& mmtStream) override;

	// MMT message
	void onMhBit(const std::shared_ptr<MmtTlv::MhBit>& mhCdt) override;
	void onMhAit(const std::shared_ptr<MmtTlv::MhAit>& mhBit) override;
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

public:
	using OutputCallback = std::function<void(const uint8_t*, size_t)>;
	void setOutputCallback(OutputCallback cb);

private:
	void writeStream(const std::shared_ptr<MmtTlv::MmtStream>& mmtStream, const std::shared_ptr<MmtTlv::MpuData>& mfuData, const std::vector<uint8_t>& data);
	void writeSubtitle(const std::shared_ptr<MmtTlv::MmtStream>& mmtStream, const B24SubtitleOutput& subtitle);

	MmtTlv::MmtTlvDemuxer& demuxer;
	OutputCallback outputCallback;
	std::unordered_map<uint16_t, uint16_t> mapService2Pid;
	std::unordered_map<uint16_t, uint8_t> mapCC;
	int tsid{-1};
	int streamCount{};
	uint64_t lastPcr{};
	ts::DuckContext duck;


};