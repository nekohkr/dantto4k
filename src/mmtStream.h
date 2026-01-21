#pragma once
#include <vector>
#include <memory>
#include <optional>
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"
#include "videoComponentDescriptor.h"
#include "mhAudioComponentDescriptor.h"

namespace MmtTlv {

class MpuProcessorBase;
class VideoComponentDescriptor;
class MhAudioComponentDescriptor;

class MmtStream final {
public:
	explicit MmtStream(uint16_t packetId)
		: packetId(packetId) {}

	MmtStream(const MmtStream&) = delete;
    MmtStream& operator=(const MmtStream&) = delete;

	MmtStream(MmtStream&&) = default;
    MmtStream& operator=(MmtStream&&) = default;

	struct TimeBase {
		int num;
		int den;
	};

	std::pair<int64_t, int64_t> getNextPtsDts();
	uint32_t getAuIndex() const { return auIndex; }
	uint16_t getMpeg2PacketId() const { return componentTag == -1 ? 0x200 + streamIndex : 0x100 + componentTag; }
	uint16_t getPacketId() const { return packetId; }
	uint32_t getAssetType() const { return assetType; }
	uint32_t getStreamIndex() const { return streamIndex; }
	int32_t getComponentTag() const { return componentTag; }
	bool getRapFlag() const { return rapFlag; }
	bool is8KVideo() const;
	bool is22_2chAudio() const;
	uint32_t getSamplingRate() const;
	std::optional<std::reference_wrapper<const VideoComponentDescriptor>> getVideoComponentDescriptor() const { return videoComponentDescriptor; }
	std::optional<std::reference_wrapper<const MhAudioComponentDescriptor>> getMhAudioComponentDescriptor() const { return mhAudioComponentDescriptor; }
	const std::vector<MpuTimestampDescriptor::Entry>& getMpuTimestamps() const { return mpuTimestamps; }
	const std::vector<MpuExtendedTimestampDescriptor::Entry>& getMpuExtendedTimestamps() const { return mpuExtendedTimestamps; }
    const TimeBase& getTimeBase() const { return timeBase; }

private:
	friend class MmtTlvDemuxer;

	std::pair<const MpuTimestampDescriptor::Entry, const MpuExtendedTimestampDescriptor::Entry> getCurrentTimestamp() const;

	uint16_t packetId;
	uint32_t assetType{0};
	uint32_t auIndex{0};
	uint32_t streamIndex{0};
	int16_t componentTag{-1};
	bool rapFlag{false};
	std::optional<uint32_t> lastMpuSequenceNumber;
	std::shared_ptr<MpuProcessorBase> mpuProcessor;
	std::optional<VideoComponentDescriptor> videoComponentDescriptor;
	std::optional<MhAudioComponentDescriptor> mhAudioComponentDescriptor;
	std::vector<MpuTimestampDescriptor::Entry> mpuTimestamps;
	std::vector<MpuExtendedTimestampDescriptor::Entry> mpuExtendedTimestamps;
	TimeBase timeBase{0, 0};

};

}
