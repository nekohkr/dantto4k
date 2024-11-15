#pragma once

namespace MmtTlv {

struct MfuData;
class MmtStream;
class Ecm;
class MhBit;
class MhCdt;
class MhEit;
class MhSdt;
class MhTot;
class Mpt;
class Plt;
class Nit;

class DemuxerHandler {
public:
	virtual ~DemuxerHandler() = default;

	// MPU data
	virtual void onVideoData(const std::shared_ptr<MmtStream> mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}
	virtual void onAudioData(const std::shared_ptr<MmtStream> mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}
	virtual void onSubtitleData(const std::shared_ptr<MmtStream> mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}
	virtual void onApplicationData(const std::shared_ptr<MmtStream> mmtStream, const std::shared_ptr<struct MfuData>& mfuData) {}

	// MMT message
	virtual void onEcm(const std::shared_ptr<Ecm>& ecm) {}
	virtual void onMhBit(const std::shared_ptr<MhBit>& mhBit) {}
	virtual void onMhCdt(const std::shared_ptr<MhCdt>& mhCdt) {}
	virtual void onMhEit(const std::shared_ptr<MhEit>& mhEit) {}
	virtual void onMhSdt(const std::shared_ptr<MhSdt>& mhSdt) {}
	virtual void onMhTot(const std::shared_ptr<MhTot>& mhTot) {}
	virtual void onMpt(const std::shared_ptr<Mpt>& mpt) {}
	virtual void onPlt(const std::shared_ptr<Plt>& plt) {}
	
	// TLV message
	virtual void onNit(const std::shared_ptr<Nit>& nit) {}
	
	
	virtual void onStreamsChanged() {}
};

}