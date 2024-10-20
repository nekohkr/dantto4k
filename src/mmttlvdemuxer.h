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
#include "mpt.h"
#include "mmtStream.h"
#include "mpuDataProcessorBase.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

enum {
	PA_MESSAGE_ID = 0x0000,
	M2_SECTION_MESSAGE = 0x8000,
	M2_SHORT_SECTION_MESSAGE = 0x8002,
};



class MpuAssembler;
class MmtTable;
class Plt;
class TlvTable;
class Ecm;
class MmtTlvDemuxer {
public:
	MmtTlvDemuxer();
	bool init();
	int processPacket(StreamBase& stream);
	void clear();

protected:
	void processMpu(Stream& stream);
	void processMpuData(Stream& stream);

	void processSignalingMessages(Stream& stream);
	void processSignalingMessage(Stream& stream);

	bool isVaildTlv(StreamBase& stream) const;

	void processPaMessage(Stream& stream);
	void processM2SectionMessage(Stream& stream);
	void processM2ShortSectionMessage(Stream& stream);

	void processTable(Stream& stream);
	void processMmtPackageTable(const std::shared_ptr<Mpt>& mpt);
	void processMpuTimestampDescriptor(const std::shared_ptr<MpuTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream> MmtStream);
	void processMpuExtendedTimestampDescriptor(const std::shared_ptr<MpuExtendedTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream> MmtStream);

	void processEcm(std::shared_ptr<Ecm> ecm);



protected:
	std::shared_ptr<MpuAssembler> getAssembler(uint16_t pid);
	std::shared_ptr<MmtStream> getStream(uint16_t pid, bool create = false);
	
	std::shared_ptr<SmartCard> smartCard;
	std::shared_ptr<AcasCard> acasCard;

	std::map<uint16_t, std::shared_ptr<MpuAssembler>> mapAssembler;
	
	TLVPacket tlv;
	CompressedIPPacket compressedIPPacket;
	Mmtp mmtp;
	Mpu mpu;

	ts::DuckContext duck;
	uint32_t streamIndex = 0;

	std::map<uint16_t, std::vector<uint8_t>> mpuData;

public:
	std::map<uint16_t, std::shared_ptr<MmtStream>> mapStream;

	std::list<std::shared_ptr<MpuData>> mpuDatas;
	std::list<std::shared_ptr<MmtTable>> tables;
	std::list<std::shared_ptr<TlvTable>> tlvTables;
};