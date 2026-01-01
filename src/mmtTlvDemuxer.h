#pragma once
#include <vector>
#include <map>
#include <list>
#include "stream.h"
#include "mmtp.h"
#include "tlv.h"
#include "mpu.h"
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"
#include "mpt.h"
#include "mmtStream.h"
#include "compressedIPPacket.h"
#include "mpuProcessorBase.h"
#include "mmtTlvStatistics.h"
#include "casHandler.h"
#include "dataUnit.h"
#include "fragmentAssembler.h"

namespace MmtTlv {

class TableBase;
class Plt;
class Ecm;
class TlvTableBase;
class DemuxerHandler;

enum class MmtMessageId {
	PaMessage = 0x0000,
	M2SectionMessage = 0x8000,
	CaMessage = 0x8001,
	M2ShortSectionMessage = 0x8002,
	DataTransmissionMessage = 0x8003,
};

enum class DemuxStatus {
	Ok = 0x0000,
	NotEnoughBuffer = 0x1000,
	NotValidTlv = 0x1001,
	WattingForEcm = 0x1002,
	Error = 0x2000,
};

class MmtTlvDemuxer {
public:
	void setDemuxerHandler(DemuxerHandler& demuxerHandler);
	void setCasHandler(std::unique_ptr<CasHandler> handler);
	DemuxStatus demux(Common::ReadStream& stream);
	void clear();
	void printStatistics() const;

private:
	bool isValidTlv(Common::ReadStream& stream) const;
	void processMpu(Common::ReadStream& stream);
	void processMfuData(Common::ReadStream& stream);
	void processSignalingMessages(Common::ReadStream& stream);
	void processSignalingMessage(Common::ReadStream& stream);
	void processPaMessage(Common::ReadStream& stream);
	void processM2SectionMessage(Common::ReadStream& stream);
	void processCaMessage(Common::ReadStream& stream);
	void processM2ShortSectionMessage(Common::ReadStream& stream);
	void processDataTransmissionMessage(Common::ReadStream& stream);
	void processTlvTable(Common::ReadStream& stream);
	void processMmtTable(Common::ReadStream& stream);
	void processMmtTableStatistics(uint8_t tableId);
	void processMmtPackageTable(const std::shared_ptr<Mpt>& mpt);
	void processMpuTimestampDescriptor(const std::shared_ptr<MpuTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream>& mmtStream);
	void processMpuExtendedTimestampDescriptor(const std::shared_ptr<MpuExtendedTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream>& mmtStream);
	void processEcm(std::shared_ptr<Ecm> ecm);

public:
	std::shared_ptr<MmtStream> getStream(uint16_t packetId);
	std::shared_ptr<MmtStream> getStreamByIdx(uint16_t idx);
	std::map<uint16_t, std::shared_ptr<MmtStream>> mapStream;

private:
	FragmentAssembler* getAssembler(uint16_t packetId);
	FragmentValidator* getFragmentValidator(uint16_t packetId);
	std::map<uint16_t, std::unique_ptr<FragmentAssembler>> mapAssembler;
	std::map<uint16_t, std::unique_ptr<FragmentValidator>> mapFragmentValidator;
	std::map<uint16_t, uint16_t> mapPacketIdByIdx;

	Tlv tlv;
	CompressedIPPacket compressedIPPacket;
	Mmtp mmtp;
	Mpu mpu;
	DataUnit dataUnit;
	std::map<uint16_t, std::vector<uint8_t>> mfuData;
	std::unique_ptr<CasHandler> casHandler;
	DemuxerHandler* demuxerHandler = nullptr;
	MmtTlvStatistics statistics;

};
}