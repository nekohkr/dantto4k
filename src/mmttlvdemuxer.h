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

#define MAKE_TAG(a, b, c, d) \
    ((static_cast<int32_t>(a) << 24) | \
     (static_cast<int32_t>(b) << 16) | \
     (static_cast<int32_t>(c) << 8)  | \
     static_cast<int32_t>(d))

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

	AVRational timeBase;
	uint32_t time_base_num = 0;
	uint32_t time_base_den = 0;

	uint32_t auIndex = 0;
	uint32_t streamIndex = 0;
	uint16_t pid = 0;

	int flags = 0;
	int offset = 0;

	std::vector<MpuTimestamp> mpuTimestamps;
	std::vector<MpuExtendedTimestamp> mpuExtendedTimestamps;
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
	SmartCard* smartCard;
	AcasCard* acasCard;
	AVPacket* packet;
	Stream* outputStream = nullptr;
	DecryptedEcm* decryptedEcm = nullptr;
	std::map<uint16_t, FragmentAssembler*> assemblerMap;
	
	FragmentAssembler* getAssembler(uint16_t pid);
	MmtpStream* getStream(uint16_t pid, bool create=false);

	TLVPacket tlv;
	CompressedIPPacket compressedIPPacket;
	Mmtp mmtp;
	Mpu mpu;

	FILE* fp;
	uint32_t last_sequence_number;
	uint32_t au_count;

	ts::DuckContext duck;
	uint32_t streamIndex = 0;

	uint8_t cc_pes = 0;
	uint32_t packetIdx = 0;

	uint64_t lastPts;
	uint64_t lastDts;


public:
	std::map<uint16_t, std::vector<uint8_t>> mpuData;
	std::map<uint16_t, MmtpStream*> streamMap;
	std::list<AVPacket*> avpackets;
	std::list<MmtTable*> tables;
	std::list<TlvTable*> tlvTables;
};