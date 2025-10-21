#pragma once
#include <memory>

namespace MmtTlv {

struct MfuData;
class MmtStream;
class Ecm;
class MhBit;
class MhAit;
class MhCdt;
class MhEit;
class MhSdt;
class MhTot;
class Mpt;
class Plt;
class Nit;
class NTPv4;

class DemuxerHandler {
public:
	virtual ~DemuxerHandler() = default;

	// MPU data
	virtual void onVideoData(const std::shared_ptr<MmtStream>& mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}
	virtual void onAudioData(const std::shared_ptr<MmtStream>& mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}
	virtual void onSubtitleData(const std::shared_ptr<MmtStream>& mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}
	virtual void onApplicationData(const std::shared_ptr<MmtStream>& mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}

	virtual void onPacketDrop(uint16_t packetId, const std::shared_ptr<MmtTlv::MmtStream>& mmtStream) {}

	// MMT-SI
	virtual void onEcm(const std::shared_ptr<Ecm>& ecm) {}
	virtual void onMhBit(const std::shared_ptr<MhBit>& mhBit) {}
	virtual void onMhAit(const std::shared_ptr<MhAit>& mhBit) {}
	virtual void onMhCdt(const std::shared_ptr<MhCdt>& mhCdt) {}
	virtual void onMhEit(const std::shared_ptr<MhEit>& mhEit) {}
	virtual void onMhSdtActual(const std::shared_ptr<MhSdt>& mhSdt) {}
	virtual void onMhTot(const std::shared_ptr<MhTot>& mhTot) {}
	virtual void onMpt(const std::shared_ptr<Mpt>& mpt) {}
	virtual void onPlt(const std::shared_ptr<Plt>& plt) {}
	
	// TLV-SI
	virtual void onNit(const std::shared_ptr<Nit>& nit) {}

	// IPv6
	virtual void onNtp(const std::shared_ptr<NTPv4>& ntp) {}
	
};

}