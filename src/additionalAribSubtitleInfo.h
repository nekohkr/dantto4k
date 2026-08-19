#pragma once
#include <cstdint>
#include <array>
#include "stream.h"
#include "ntpTimestamp.h"

namespace MmtTlv {

enum class SubtitleResolution : uint8_t {
	Resolution2K = 0, // 1920 x 1080
	Resolution4K = 1, // 3840 x 2160
	Resolution8K = 2, // 7680 x 4320
};

class AdditionalAribSubtitleInfo {
public:
	bool unpack(Common::ReadStream& stream);

	uint8_t subtitleTag{};
	uint8_t subtitle_info_version{};
	bool start_mpu_sequence_number_flag{};
	std::array<char, 3> languageCode;
	uint8_t type{};
	uint8_t subtitleFormat{};
	uint8_t opm{};
	uint8_t tmd{};
	uint8_t dmf{};
	SubtitleResolution resolution{SubtitleResolution::Resolution4K};
	uint8_t compressionType{};
	uint32_t startMpuSequenceNumber{};
	NtpTimestamp referenceStartTime;
	uint8_t referenceStartTimeLeapIndicator{};

};

}
