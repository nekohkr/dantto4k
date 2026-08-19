#include "additionalAribSubtitleInfo.h"

namespace MmtTlv {

bool AdditionalAribSubtitleInfo::unpack(Common::ReadStream& stream) {
	try {
		subtitleTag = stream.get8U();

		uint8_t uint8 = stream.get8U();
		subtitle_info_version = (uint8 & 0b11110000) >> 4;
		start_mpu_sequence_number_flag = (uint8 & 0b00001000) >> 3;

		stream.read(languageCode.data(), languageCode.size());

		uint8 = stream.get8U();
		type = (uint8 & 0b11000000) >> 6;
		subtitleFormat = (uint8 & 0b00111100) >> 2;
		opm = uint8 & 0b00000011;

		uint8 = stream.get8U();
		tmd = (uint8 & 0b11110000) >> 4;
		dmf = uint8 & 0b00001111;

		uint8 = stream.get8U();
		resolution = static_cast<SubtitleResolution>((uint8 & 0b11110000) >> 4);
		compressionType = uint8 & 0b00001111;

		startMpuSequenceNumber = 0;
		if (start_mpu_sequence_number_flag) {
			startMpuSequenceNumber = stream.getBe32U();
		}

		referenceStartTime = {};
		referenceStartTimeLeapIndicator = 0;
		if (tmd == 0b0010) {
			if (!referenceStartTime.unpack(stream)) {
				return false;
			}
			referenceStartTimeLeapIndicator = stream.get8U() >> 6;
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}
