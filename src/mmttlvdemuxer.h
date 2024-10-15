#pragma once
#define _WINSOCKAPI_
#include <windows.h>
#include <vector>
#include "stream.h"
#include "acascard.h"
#include <map>
#include <list>
#include "mmtp.h"
#include "tlv.h"
#include <tsduck.h>
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

constexpr int32_t makeTag(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
	return (a << 24) | (b << 16) | (c << 8) | d;
}

enum {
	PA_MESSAGE_ID = 0x0000,
	M2_SECTION_MESSAGE = 0x8000,
	M2_SHORT_SECTION_MESSAGE = 0x8002,
};

enum FragmentationIndicator {
	NOT_FRAGMENTED = 0b00,
	FIRST_FRAGMENT = 0b01,
	MIDDLE_FRAGMENT = 0b10,
	LAST_FRAGMENT = 0b11,
};

enum class ASSEMBLER_STATE
{
	INIT,
	NOT_STARTED,
	IN_FRAGMENT,
	SKIP,
};

class MmtpStream {
public:
	enum AVMediaType codecType;
	enum AVCodecID codecId = AV_CODEC_ID_NONE;
	uint32_t codecTag = 0;

	uint32_t lastMpuSequenceNumber = 0;
	uint32_t auIndex = 0;
	uint32_t streamIndex = 0;
	uint16_t pid = 0;

	AVRational timeBase;

	int flags = 0;

	std::vector<MpuTimestampDescriptor::Entry> mpuTimestamps;
	std::vector<MpuExtendedTimestampDescriptor::Entry> mpuExtendedTimestamps;
	std::vector<uint8_t> pendingData;
};

class FragmentAssembler {
public:
	bool assemble(std::vector<uint8_t> fragment, uint8_t fragmentationIndicator, uint32_t packetSequenceNumber);
	void checkState(uint32_t packetSequenceNumber);
	void clear();

	ASSEMBLER_STATE state = ASSEMBLER_STATE::INIT;
	std::vector<uint8_t> data;
	uint32_t last_seq = 0;
};

class MmtTable;
class Plt;
class TlvTable;
class Ecm;
class MmtTlvDemuxer {
public:
	bool init();
	int processPacket(Stream& stream);
	void clear();

protected:
	void processMpu(Stream& stream);
	void processMpuData(Stream& stream);

	void processSignalingMessages(Stream& stream);
	void processSignalingMessage(Stream& stream);

	bool isVaildTlv(Stream& stream) const;

	void processPaMessage(Stream& stream);
	void processM2SectionMessage(Stream& stream);
	void processM2ShortSectionMessage(Stream& stream);

	void processTable(Stream& stream);
	void processMmtPackageTable(Stream& stream);
	void processMptDescriptor(Stream& stream, MmtpStream* mmtpStream);
	void processMpuTimestampDescriptor(Stream& stream, MmtpStream* mmtpStream);
	void processMpuExtendedTimestampDescriptor(Stream& stream, MmtpStream* mmtpStream);

	void processEcm(Ecm* ecm);

	void clearTables();


protected:
	FragmentAssembler* getAssembler(uint16_t pid);
	MmtpStream* getStream(uint16_t pid, bool create = false);
	std::pair<int64_t, int64_t> calcPtsDts(MmtpStream* mmtpStream, MpuTimestampDescriptor::Entry& timestamp, MpuExtendedTimestampDescriptor::Entry& extendedTimestamp);

	SmartCard* smartCard = nullptr;
	AcasCard* acasCard = nullptr;
	std::map<uint16_t, FragmentAssembler*> assemblerMap;
	
	TLVPacket tlv;
	CompressedIPPacket compressedIPPacket;
	Mmtp mmtp;
	Mpu mpu;

	ts::DuckContext duck;
	uint32_t streamIndex = 0;

	std::map<uint16_t, std::vector<uint8_t>> mpuData;

public:
	std::map<uint16_t, MmtpStream*> streamMap;
	std::list<AVPacket*> avpackets;
	std::list<MmtTable*> tables;
	std::list<TlvTable*> tlvTables;
};