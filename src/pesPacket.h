#pragma once
#include <cstdint>
#include <vector>

constexpr uint8_t STREAM_ID_PROGRAM_STREAM_MAP =       0xbc;
constexpr uint8_t STREAM_ID_PRIVATE_STREAM_1 =         0xbd;
constexpr uint8_t STREAM_ID_PADDING_STREAM =           0xbe;
constexpr uint8_t STREAM_ID_PRIVATE_STREAM_2 =         0xbf;
constexpr uint8_t STREAM_ID_AUDIO_STREAM_0 =           0xc0;
constexpr uint8_t STREAM_ID_VIDEO_STREAM_0 =           0xe0;
constexpr uint8_t STREAM_ID_ECM_STREAM =               0xf0;
constexpr uint8_t STREAM_ID_EMM_STREAM =               0xf1;
constexpr uint8_t STREAM_ID_DSMCC_STREAM =             0xf2;
constexpr uint8_t STREAM_ID_TYPE_E_STREAM =            0xf8;
constexpr uint8_t STREAM_ID_METADATA_STREAM =          0xfc;
constexpr uint8_t STREAM_ID_EXTENDED_STREAM_ID =       0xfd;
constexpr uint8_t STREAM_ID_PROGRAM_STREAM_DIRECTORY = 0xff;

constexpr uint64_t NOPTS_VALUE = 0x8000000000000000;

constexpr uint8_t componentTagToStreamId(uint8_t tag) {
	if (tag >= 0 && tag <= 0x0F) {
		return STREAM_ID_VIDEO_STREAM_0 + tag;
	}
	
	if (tag >= 0x10 && tag <= 0x2F) {
		return STREAM_ID_AUDIO_STREAM_0 + (tag - 0x10);
	}

	if (tag == 0x30) {
		return STREAM_ID_PRIVATE_STREAM_1;
	}

	if (tag == 0x38) {
		return STREAM_ID_PRIVATE_STREAM_2;
	}

	return STREAM_ID_PRIVATE_STREAM_1;
}

class PESPacket {
public:
	bool pack(std::vector<uint8_t>& output);
	void setPts(uint64_t pts) { this->pts = pts; }
	void setDts(uint64_t dts) { this->dts = dts; }
	void setStreamId(uint8_t streamId) { this->streamId = streamId; };
	void setDataAlignmentIndicator(bool dataAlignmentIndicator) { this->dataAlignmentIndicator = dataAlignmentIndicator; }
	uint64_t getPts() const { return pts; }
	uint64_t getDts() const { return dts; }
	uint8_t setStreamId() const { return streamId; }
	bool getDataAlignmentIndicator() const { return dataAlignmentIndicator; }

    std::vector<uint8_t> pesPrivateData;
	std::vector<uint8_t> payload;

private:
	uint8_t streamId{};
	bool dataAlignmentIndicator{};

	uint64_t pts{NOPTS_VALUE};
	uint64_t dts{NOPTS_VALUE};

};