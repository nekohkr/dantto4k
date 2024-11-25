#pragma once
#define _WINSOCKAPI_
#include <windows.h>
#include <vector>
#include <map>
#include <list>
#include <tsduck.h>

#include "stream.h"
#include "acascard.h"
#include "mmt.h"
#include "tlv.h"
#include "mpu.h"
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"
#include "mpt.h"
#include "mmtStream.h"
#include "compressedIPPacket.h"
#include "mfuDataProcessorBase.h"

namespace MmtTlv {

enum class MmtMessageId {
	PaMessage = 0x0000,
	M2SectionMessage = 0x8000,
	M2ShortSectionMessage = 0x8002,
};

class FragmentAssembler;
class TableBase;
class Plt;
class Ecm;
class TlvTableBase;
class DemuxerHandler;

class MmtTlvDemuxer {
public:
	MmtTlvDemuxer();
	bool init();
	void setDemuxerHandler(DemuxerHandler& demuxerHandler);
	int processPacket(Common::ReadStream& stream);
	void clear();

private:
	void processMpu(Common::ReadStream& stream);
	void processMfuData(Common::ReadStream& stream);

	void processSignalingMessages(Common::ReadStream& stream);
	void processSignalingMessage(Common::ReadStream& stream);

	bool isVaildTlv(Common::ReadStream& stream) const;

	void processPaMessage(Common::ReadStream& stream);
	void processM2SectionMessage(Common::ReadStream& stream);
	void processM2ShortSectionMessage(Common::ReadStream& stream);
	
	void processTlvTable(Common::ReadStream& stream);
	void processMmtTable(Common::ReadStream& stream);

	void processMmtPackageTable(const std::shared_ptr<Mpt>& mpt);
	void processMpuTimestampDescriptor(const std::shared_ptr<MpuTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream>& mmtStream);
	void processMpuExtendedTimestampDescriptor(const std::shared_ptr<MpuExtendedTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream>& mmtStream);

	void processEcm(std::shared_ptr<Ecm> ecm);

public:
	std::shared_ptr<MmtStream> getStream(uint16_t pid);

	std::map<uint16_t, std::shared_ptr<MmtStream>> mapStream;
	std::map<uint16_t, std::shared_ptr<MmtStream>> mapStreamByStreamIdx;

private:
	std::shared_ptr<FragmentAssembler> getAssembler(uint16_t pid);

	std::shared_ptr<Acas::SmartCard> smartCard;
	std::unique_ptr<Acas::AcasCard> acasCard;

	std::map<uint16_t, std::shared_ptr<FragmentAssembler>> mapAssembler;

	Tlv tlv;
	CompressedIPPacket compressedIPPacket;
	Mmt mmt;
	Mpu mpu;

	ts::DuckContext duck;

	std::map<uint16_t, std::vector<uint8_t>> mfuData;
	DemuxerHandler* demuxerHandler = nullptr;
};
}