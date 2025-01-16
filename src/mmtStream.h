#pragma once
#include <vector>
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"

namespace MmtTlv {

class MfuDataProcessorBase;
class VideoComponentDescriptor;
class MhAudioComponentDescriptor;

class MmtStream final {
public:
	MmtStream(uint16_t packetId)
		: packetId(packetId) {}

	MmtStream(const MmtStream&) = delete;
    MmtStream& operator=(const MmtStream&) = delete;

	MmtStream(MmtStream&&) = default;
    MmtStream& operator=(MmtStream&&) = default;

	std::pair<int64_t, int64_t> getNextPtsDts();
	uint32_t getAuIndex() const { return auIndex; }
	uint16_t getMpeg2PacketId() const { return componentTag == -1 ? 0x200 + streamIndex : 0x100 + componentTag; }
	uint16_t getPacketId() const { return packetId; }
	uint32_t getAssetType() const { return assetType; }
	uint32_t getStreamIndex() const { return streamIndex; }
	int32_t getComponentTag() const { return componentTag; }
	bool GetRapFlag() const { return rapFlag; }
	bool Is8KVideo() const;
	bool Is22_2chAudio() const;
	uint32_t getSamplingRate() const;

	const std::shared_ptr<VideoComponentDescriptor>& getVideoComponentDescriptor() const { return videoComponentDescriptor; }
	const std::shared_ptr<MhAudioComponentDescriptor>& getMhAudioComponentDescriptor() const { return mhAudioComponentDescriptor; }

	struct TimeBase {
		int num;
		int den;
	};

	struct TimeBase timeBase;

private:
	friend class MmtTlvDemuxer;

	std::pair<const MpuTimestampDescriptor::Entry, const MpuExtendedTimestampDescriptor::Entry> getCurrentTimestamp() const;
	
	uint16_t packetId = 0;
	
	uint32_t assetType = 0;
	uint32_t lastMpuSequenceNumber = 0;
	uint32_t auIndex = 0;
	uint32_t streamIndex = 0;

	int16_t componentTag = -1;

	bool rapFlag = false;

	std::vector<MpuTimestampDescriptor::Entry> mpuTimestamps;
	std::vector<MpuExtendedTimestampDescriptor::Entry> mpuExtendedTimestamps;
	std::shared_ptr<MfuDataProcessorBase> mfuDataProcessor;
	
	std::shared_ptr<VideoComponentDescriptor> videoComponentDescriptor;
	std::shared_ptr<MhAudioComponentDescriptor> mhAudioComponentDescriptor;
};

}
