#include "acascard.h"
#include "dataUnit.h"
#include "ecm.h"
#include "m2SectionMessage.h"
#include "m2ShortSectionMessage.h"
#include "mhCdt.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "mhTot.h"
#include "mhBit.h"
#include "mhAit.h"
#include "mmtStream.h"
#include "mmtTlvDemuxer.h"
#include "mpt.h"
#include "fragmentAssembler.h"
#include "mfuDataProcessorFactory.h"
#include "nit.h"
#include "paMessage.h"
#include "plt.h"
#include "signalingMessage.h"
#include "smartcard.h"
#include "stream.h"
#include "mmtTableFactory.h"
#include "tlvTableFactory.h"
#include "demuxerHandler.h"
#include "mhStreamIdentificationDescriptor.h"
#include "videoMfuDataProcessor.h"
#include "videoComponentDescriptor.h"
#include "mhAudioComponentDescriptor.h"
#include "ipv6.h"
#include "ntp.h"
#include "dataTransmissionMessage.h"
#include "caMessage.h"
#include <algorithm>

namespace MmtTlv {

MmtTlvDemuxer::MmtTlvDemuxer()
{
    smartCard = std::make_shared<Acas::SmartCard>();
    acasCard = std::make_unique<Acas::AcasCard>(smartCard);
}

bool MmtTlvDemuxer::init()
{
    try {
        smartCard->init();
        smartCard->connect();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }

    return true;
}

void MmtTlvDemuxer::setDemuxerHandler(DemuxerHandler& demuxerHandler)
{
    this->demuxerHandler = &demuxerHandler;
}

void MmtTlvDemuxer::setSmartCardReaderName(const std::string& smartCardReaderName) {
    
    smartCard->setSmartCardReaderName(smartCardReaderName);
}

DemuxStatus MmtTlvDemuxer::demux(Common::ReadStream& stream)
{
    size_t cur = stream.getCur();

    if (stream.leftBytes() < 4) {
        return DemuxStatus::NotEnoughBuffer;
    }

    if (!isVaildTlv(stream)) {
        stream.skip(1);
        return DemuxStatus::NotValidTlv;
    }

    if (!tlv.unpack(stream)) {
        stream.setCur(cur);
        return DemuxStatus::NotEnoughBuffer;
    }

    if (stream.leftBytes() < tlv.getDataLength()) {
        stream.setCur(cur);
        return DemuxStatus::NotEnoughBuffer;
    }

    statistics.tlvPacketCount++;

    Common::ReadStream tlvDataStream(tlv.getData());

    switch (tlv.getPacketType()) {
    case TlvPacketType::Ipv4Packet:
    {
        statistics.tlvIpv4PacketCount++;
        break;
    }
    case TlvPacketType::Ipv6Packet:
    {
        statistics.tlvIpv6PacketCount++;

        IPv6Header ipv6(false);
        if (!ipv6.unpack(tlvDataStream)) {
            break;
        }

        if (ipv6.nexthdr == IPv6::PROTOCOL_UDP) {
            UDPHeader udpHeader;
            if (!udpHeader.unpack(tlvDataStream)) {
                break;
            }

            // NTP
            if (udpHeader.destination_port == IPv6::PORT_NTP) {
                NTPv4 ntp;
                if (!ntp.unpack(tlvDataStream)) {
                    break;
                }

                demuxerHandler->onNtp(std::make_shared<NTPv4>(ntp));
            }
        }
        break;
    }
    case TlvPacketType::HeaderCompressedIpPacket:
    {
        statistics.tlvHeaderCompressedIpPacketCount++;

        if (!compressedIPPacket.unpack(tlvDataStream)) {
            break;
        }
        
        if (!mmt.unpack(tlvDataStream)) {
            break;
        }
        
        auto mmtStat = statistics.getMmtStat(mmt.packetId);
        if (mmtStat->count == 0) {
            mmtStat->lastPacketSequenceNumber = mmt.packetSequenceNumber;
            mmtStat->count++;
        }
        else {
            auto mmtStat = statistics.getMmtStat(mmt.packetId);
            if (mmtStat->lastPacketSequenceNumber + 1 != mmt.packetSequenceNumber) {
                mmtStat->drop++;
            }
            mmtStat->lastPacketSequenceNumber = mmt.packetSequenceNumber;
            mmtStat->count++;
        }

        if (mmt.extensionHeaderScrambling.has_value()) {
            if (mmt.extensionHeaderScrambling->encryptionFlag == EncryptionFlag::ODD ||
                mmt.extensionHeaderScrambling->encryptionFlag == EncryptionFlag::EVEN) {
                auto lastEcm = acasCard->getLastEcm();
                if (!lastEcm) {
                    return DemuxStatus::WattingForEcm;
                }
                
                mmt.decryptPayload(*lastEcm);
            }
        }

        Common::ReadStream mmtpPayloadStream(mmt.payload);
        switch (mmt.payloadType) {
        case PayloadType::Mpu:
            processMpu(mmtpPayloadStream);
            break;
        case PayloadType::ContainsOneOrMoreControlMessage:
            processSignalingMessages(mmtpPayloadStream);
            break;
        default:
            break;
        }
        break;
    }
    case TlvPacketType::TransmissionControlSignalPacket:
    {
        statistics.tlvTransmissionControlSignalPacketCount++;
        processTlvTable(tlvDataStream);
        break;
    }
    case TlvPacketType::NullPacket:
    {
        statistics.tlvNullPacketCount++;
        break;
    }
    default:
    {
        statistics.tlvUndefinedCount++;
    }
    }

    return DemuxStatus::Ok;
}

void MmtTlvDemuxer::processPaMessage(Common::ReadStream& stream)
{
    PaMessage message;
    if (!message.unpack(stream)) {
        return;
    }

    Common::ReadStream nstream(message.table);
    while (!nstream.isEof()) {
        processMmtTable(nstream);
    }
}

void MmtTlvDemuxer::processM2SectionMessage(Common::ReadStream& stream)
{
    M2SectionMessage message;
    if (!message.unpack(stream)) {
        return;
    }

    processMmtTable(stream);
}

void MmtTlvDemuxer::processCaMessage(Common::ReadStream& stream)
{
    CaMessage message;
    if (!message.unpack(stream)) {
        return;
    }
    
    processMmtTable(stream);
}

void MmtTlvDemuxer::processM2ShortSectionMessage(Common::ReadStream& stream)
{
    M2ShortSectionMessage message;
    if (!message.unpack(stream)) {
        return;
    }

    processMmtTable(stream);
}

void MmtTlvDemuxer::processDataTransmissionMessage(Common::ReadStream& stream)
{
    DataTransmissionMessage message;
    if (!message.unpack(stream)) {
        return;
    }
    
    processMmtTable(stream);
}

void MmtTlvDemuxer::processTlvTable(Common::ReadStream& stream)
{
    if (stream.leftBytes() < 2) {
        return;
    }

    uint8_t tableId = stream.peek8U();
    const auto table = TlvTableFactory::create(tableId);
    if (table == nullptr) {
        return;
    }

    if (!table->unpack(stream)) {
        return;
    }

    switch (tableId) {
    case TlvTableId::Nit:
        if (demuxerHandler) {
            demuxerHandler->onNit(std::dynamic_pointer_cast<Nit>(table));
        }
        break;
    }
}

void MmtTlvDemuxer::processMmtTable(Common::ReadStream& stream)
{
    uint8_t tableId = stream.peek8U();
    processMmtTableStatistics(tableId);

    const auto table = MmtTableFactory::create(tableId);
    if (table == nullptr) {
        stream.skip(stream.leftBytes());
        return;
    }

    table->unpack(stream);

    switch (tableId) {
    case MmtTableId::Mpt:
    {
        processMmtPackageTable(std::dynamic_pointer_cast<Mpt>(table));
        break;
    }
    case MmtTableId::Ecm_0:
    {
        processEcm(std::dynamic_pointer_cast<Ecm>(table));
        break;
    }
    }
    
    if (demuxerHandler) {
        switch (tableId) {
        case MmtTableId::Ecm_0:
            demuxerHandler->onEcm(std::dynamic_pointer_cast<Ecm>(table));
            break;
        case MmtTableId::MhCdt:
            demuxerHandler->onMhCdt(std::dynamic_pointer_cast<MhCdt>(table));
            break;
        case MmtTableId::MhEitPf:
        case MmtTableId::MhEitS_0:
        case MmtTableId::MhEitS_1:
        case MmtTableId::MhEitS_2:
        case MmtTableId::MhEitS_3:
        case MmtTableId::MhEitS_4:
        case MmtTableId::MhEitS_5:
        case MmtTableId::MhEitS_6:
        case MmtTableId::MhEitS_7:
        case MmtTableId::MhEitS_8:
        case MmtTableId::MhEitS_9:
        case MmtTableId::MhEitS_10:
        case MmtTableId::MhEitS_11:
        case MmtTableId::MhEitS_12:
        case MmtTableId::MhEitS_13:
        case MmtTableId::MhEitS_14:
        case MmtTableId::MhEitS_15:
            demuxerHandler->onMhEit(std::dynamic_pointer_cast<MhEit>(table));
            break;
        case MmtTableId::MhSdtActual:
            demuxerHandler->onMhSdtActual(std::dynamic_pointer_cast<MhSdt>(table));
            break;
        case MmtTableId::MhTot:
            demuxerHandler->onMhTot(std::dynamic_pointer_cast<MhTot>(table));
            break;
        case MmtTableId::Mpt:
            demuxerHandler->onMpt(std::dynamic_pointer_cast<Mpt>(table));
            break;
        case MmtTableId::Plt:
            demuxerHandler->onPlt(std::dynamic_pointer_cast<Plt>(table));
            break;
        case MmtTableId::MhBit:
            demuxerHandler->onMhBit(std::dynamic_pointer_cast<MhBit>(table));
            break;
        case MmtTableId::MhAit:
            demuxerHandler->onMhAit(std::dynamic_pointer_cast<MhAit>(table));
            break;
        }
    }
}

void MmtTlvDemuxer::processMmtTableStatistics(uint8_t tableId)
{
    switch (tableId) {
    case MmtTableId::Pat:
        statistics.getMmtStat(mmt.packetId)->setName("PAT");
        break;
    case MmtTableId::Ecm_0:
        statistics.getMmtStat(mmt.packetId)->setName("ECM");
        break;
    case MmtTableId::Ecm_1:
        statistics.getMmtStat(mmt.packetId)->setName("ECM");
        break;
    case MmtTableId::MhCdt:
        statistics.getMmtStat(mmt.packetId)->setName("MH-CDT");
        break;
    case MmtTableId::MhEitPf:
        statistics.getMmtStat(mmt.packetId)->setName("MH-EIT");
        break;
    case MmtTableId::MhEitS_0:
    case MmtTableId::MhEitS_1:
    case MmtTableId::MhEitS_2:
    case MmtTableId::MhEitS_3:
    case MmtTableId::MhEitS_4:
    case MmtTableId::MhEitS_5:
    case MmtTableId::MhEitS_6:
    case MmtTableId::MhEitS_7:
    case MmtTableId::MhEitS_8:
    case MmtTableId::MhEitS_9:
    case MmtTableId::MhEitS_10:
    case MmtTableId::MhEitS_11:
    case MmtTableId::MhEitS_12:
    case MmtTableId::MhEitS_13:
    case MmtTableId::MhEitS_14:
    case MmtTableId::MhEitS_15:
        statistics.getMmtStat(mmt.packetId)->setName("MH-EIT");
        break;
    case MmtTableId::MhSdtActual:
        statistics.getMmtStat(mmt.packetId)->setName("MH-SDT");
        break;
    case MmtTableId::MhSdtOther:
        statistics.getMmtStat(mmt.packetId)->setName("MH-SDT");
        break;
    case MmtTableId::MhTot:
        statistics.getMmtStat(mmt.packetId)->setName("MH-TOT");
        break;
    case MmtTableId::Mpt:
        statistics.getMmtStat(mmt.packetId)->setName("MPT");
        break;
    case MmtTableId::Plt:
        statistics.getMmtStat(mmt.packetId)->setName("PLT");
        break;
    case MmtTableId::MhBit:
        statistics.getMmtStat(mmt.packetId)->setName("MH-BIT");
        break;
    case MmtTableId::Lct:
        statistics.getMmtStat(mmt.packetId)->setName("LCT");
        break;
    case MmtTableId::Emm_0:
        statistics.getMmtStat(mmt.packetId)->setName("EMM");
        break;
    case MmtTableId::Emm_1:
        statistics.getMmtStat(mmt.packetId)->setName("EMM");
        break;
    case MmtTableId::Cat:
        statistics.getMmtStat(mmt.packetId)->setName("CAT");
        break;
    case MmtTableId::Dcm:
        statistics.getMmtStat(mmt.packetId)->setName("DCM");
        break;
    case MmtTableId::Dmm:
        statistics.getMmtStat(mmt.packetId)->setName("DMM");
        break;
    case MmtTableId::MhSdtt:
        statistics.getMmtStat(mmt.packetId)->setName("MH-SDTT");
        break;
    case MmtTableId::MhAit:
        statistics.getMmtStat(mmt.packetId)->setName("MH-AIT");
        break;
    case MmtTableId::Ddmt:
        statistics.getMmtStat(mmt.packetId)->setName("DDMT");
        break;
    case MmtTableId::Damt:
        statistics.getMmtStat(mmt.packetId)->setName("DAMT");
        break;
    case MmtTableId::Dcct:
        statistics.getMmtStat(mmt.packetId)->setName("DCCT");
        break;
    case MmtTableId::Emt:
        statistics.getMmtStat(mmt.packetId)->setName("EMT");
        break;
    }
}

void MmtTlvDemuxer::processMmtPackageTable(const std::shared_ptr<Mpt>& mpt)
{
    // Remove streams that do not exist in the MPT
    std::map<uint16_t, uint32_t> mapMpt; // packetId, assetType
    for (auto& asset : mpt->assets) {
        for (auto& locationInfo : asset.locationInfos) {
            if (locationInfo.locationType == 0) {
                mapMpt[locationInfo.packetId] = asset.assetType;
            }
        }
    }

    if (mapMpt.size()) {
        for (auto it = mapStream.begin(); it != mapStream.end(); ) {
            auto mptIt = mapMpt.find(it->first);
            if (mptIt != mapMpt.end()) {
                if (mptIt->second != it->second->assetType) {
                    it = mapStream.erase(it);
                }
                else {
                    ++it;
                }
            }
            else {
                it = mapStream.erase(it);
            }
        }
    }

    mapStreamByStreamIdx.clear();

    int streamIndex = 0;
    for (const auto& asset : mpt->assets) {
        std::shared_ptr<MmtStream> mmtStream;

        for (const auto& locationInfo : asset.locationInfos) {
            if (locationInfo.locationType == 0) {
                if (asset.assetType == AssetType::hev1 ||
                    asset.assetType == AssetType::mp4a ||
                    asset.assetType == AssetType::stpp ||
                    asset.assetType == AssetType::aapp) {
                    mmtStream = getStream(locationInfo.packetId);
                    if (!mmtStream) {
                        mmtStream = std::make_shared<MmtStream>(locationInfo.packetId);
                        mapStream[locationInfo.packetId] = mmtStream;
                    }
                    mmtStream->assetType = asset.assetType;
                    mmtStream->streamIndex = streamIndex;

                    if (!mmtStream->mfuDataProcessor) {
                        mmtStream->mfuDataProcessor = MfuDataProcessorFactory::create(mmtStream->assetType);
                    }

                    mapStreamByStreamIdx[streamIndex] = mmtStream;
                    statistics.getMmtStat(locationInfo.packetId)->assetType = asset.assetType;
                    ++streamIndex;
                }
            }
        }

        if (!mmtStream) {
            continue;
        }

        for (const auto& descriptor : asset.descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MpuTimestampDescriptor::kDescriptorTag:
            {
                processMpuTimestampDescriptor(std::dynamic_pointer_cast<MpuTimestampDescriptor>(descriptor), mmtStream);
                break;
            }
            case MpuExtendedTimestampDescriptor::kDescriptorTag:
            {
                processMpuExtendedTimestampDescriptor(std::dynamic_pointer_cast<MpuExtendedTimestampDescriptor>(descriptor), mmtStream);
                break;
            }
            case MhStreamIdentificationDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhStreamIdentificationDescriptor>(descriptor);
                mmtStream->componentTag = mmtDescriptor->componentTag;
                break;
            }
            case VideoComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::VideoComponentDescriptor>(descriptor);
                mmtStream->videoComponentDescriptor = mmtDescriptor;

                statistics.getMmtStat(mmtStream->packetId)->videoResolution = mmtDescriptor->videoResolution;
                statistics.getMmtStat(mmtStream->packetId)->videoAspectRatio = mmtDescriptor->videoAspectRatio;
                break;
            }
            case MhAudioComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhAudioComponentDescriptor>(descriptor);
                mmtStream->mhAudioComponentDescriptor = mmtDescriptor;
                
                statistics.getMmtStat(mmtStream->packetId)->audioComponentType = mmtDescriptor->componentType;
                statistics.getMmtStat(mmtStream->packetId)->audioSamplingRate = mmtDescriptor->samplingRate;
                break;
            }
            }
        }
    }
}

void MmtTlvDemuxer::processMpuTimestampDescriptor(const std::shared_ptr<MpuTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream>& mmtStream)
{
    for (const auto& ts : descriptor->entries) {
        bool find = false;
        for (size_t i = 0; i < mmtStream->mpuTimestamps.size(); i++) {
            if (mmtStream->mpuTimestamps[i].mpuSequenceNumber == ts.mpuSequenceNumber) {
                mmtStream->mpuTimestamps[i].mpuPresentationTime = ts.mpuPresentationTime;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        for (size_t i = 0; i < mmtStream->mpuTimestamps.size(); i++) {
            if (mmtStream->mpuTimestamps[i].mpuSequenceNumber < mmtStream->lastMpuSequenceNumber) {
                mmtStream->mpuTimestamps[i].mpuSequenceNumber = ts.mpuSequenceNumber;
                mmtStream->mpuTimestamps[i].mpuPresentationTime = ts.mpuPresentationTime;
                find = true;
                break;
            }
        }

        if (find) {
            continue;
        }

        if (mmtStream->mpuTimestamps.size() >= 100) {
            auto minElement = std::min_element(mmtStream->mpuTimestamps.begin(), mmtStream->mpuTimestamps.end(), 
                [](const auto& lhs, const auto& rhs) {
                    return lhs.mpuSequenceNumber < rhs.mpuSequenceNumber;
                });

            if (minElement != mmtStream->mpuTimestamps.end()) {
                minElement->mpuSequenceNumber = ts.mpuSequenceNumber;
                minElement->mpuPresentationTime = ts.mpuPresentationTime;
            }
        }
        else {
            mmtStream->mpuTimestamps.push_back(ts);
        }
    }
}

void MmtTlvDemuxer::processMpuExtendedTimestampDescriptor(const std::shared_ptr<MpuExtendedTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream>& mmtStream)
{
    if (descriptor->timescaleFlag) {
        mmtStream->timeBase.num = 1;
        mmtStream->timeBase.den = descriptor->timescale;
    }

    for (const auto& ts : descriptor->entries) {
        if (mmtStream->lastMpuSequenceNumber > ts.mpuSequenceNumber)
            continue;

        bool find = false;
        for (size_t i = 0; i < mmtStream->mpuExtendedTimestamps.size(); i++) {
            if (mmtStream->mpuExtendedTimestamps[i].mpuSequenceNumber == ts.mpuSequenceNumber) {
                mmtStream->mpuExtendedTimestamps[i].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
                mmtStream->mpuExtendedTimestamps[i].numOfAu = ts.numOfAu;
                mmtStream->mpuExtendedTimestamps[i].ptsOffsets = ts.ptsOffsets;
                mmtStream->mpuExtendedTimestamps[i].dtsPtsOffsets = ts.dtsPtsOffsets;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        for (size_t i = 0; i < mmtStream->mpuExtendedTimestamps.size(); i++) {
            if (mmtStream->mpuExtendedTimestamps[i].mpuSequenceNumber < mmtStream->lastMpuSequenceNumber) {
                mmtStream->mpuExtendedTimestamps[i].mpuSequenceNumber = ts.mpuSequenceNumber;
                mmtStream->mpuExtendedTimestamps[i].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
                mmtStream->mpuExtendedTimestamps[i].numOfAu = ts.numOfAu;
                mmtStream->mpuExtendedTimestamps[i].ptsOffsets = ts.ptsOffsets;
                mmtStream->mpuExtendedTimestamps[i].dtsPtsOffsets = ts.dtsPtsOffsets;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        if (mmtStream->mpuExtendedTimestamps.size() >= 100) {
            auto minElement = std::min_element(mmtStream->mpuExtendedTimestamps.begin(), mmtStream->mpuExtendedTimestamps.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.mpuSequenceNumber < rhs.mpuSequenceNumber;
                });

            if (minElement != mmtStream->mpuExtendedTimestamps.end()) {
                minElement->mpuSequenceNumber = ts.mpuSequenceNumber;
                minElement->mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
                minElement->numOfAu = ts.numOfAu;
                minElement->ptsOffsets = ts.ptsOffsets;
                minElement->dtsPtsOffsets = ts.dtsPtsOffsets;
            }
        }
        else {
            mmtStream->mpuExtendedTimestamps.push_back(ts);
        }
    }
}

void MmtTlvDemuxer::processEcm(std::shared_ptr<Ecm> ecm)
{
    try {
        acasCard->processEcm(ecm->ecmData);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
}

void MmtTlvDemuxer::clear()
{
    mapAssembler.clear();
    mfuData.clear();
    mapStream.clear();
    mapStreamByStreamIdx.clear();
    acasCard->clear();
    statistics.clear();
}

void MmtTlvDemuxer::release()
{
    smartCard->release();
}

void MmtTlvDemuxer::printStatistics() const
{
    statistics.print();
}

std::shared_ptr<FragmentAssembler> MmtTlvDemuxer::getAssembler(uint16_t packetId)
{
    if (mapAssembler.find(packetId) == mapAssembler.end()) {
        auto assembler = std::make_shared<FragmentAssembler>();
        mapAssembler[packetId] = assembler;
        return assembler;
    }
    else {
        return mapAssembler[packetId];
    }
}

std::shared_ptr<MmtStream> MmtTlvDemuxer::getStream(uint16_t packetId)
{
    if (mapStream.find(packetId) == mapStream.end()) {
        return nullptr;
    }
    else {
        return mapStream[packetId];
    }
}

void MmtTlvDemuxer::processMpu(Common::ReadStream& stream)
{
    if (!mpu.unpack(stream)) {
        return;
    }

    auto assembler = getAssembler(mmt.packetId);
    Common::ReadStream nstream(mpu.payload);
    std::shared_ptr<MmtStream> mmtStream = getStream(mmt.packetId);

    if (!mmtStream) {
        return;
    }

    if (mpu.aggregateFlag && mpu.fragmentationIndicator != FragmentationIndicator::NotFragmented) {
        return;
    }

    if (mpu.fragmentType != FragmentType::Mfu) {
        return;
    }

    if (assembler->state == FragmentAssembler::State::Init) {
        mmtStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
    }
    else if (mpu.mpuSequenceNumber == mmtStream->lastMpuSequenceNumber + 1) {
        mmtStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
        mmtStream->auIndex = 0;
    }
    else if (mpu.mpuSequenceNumber != mmtStream->lastMpuSequenceNumber) {
        assembler->state = FragmentAssembler::State::Init;
        return;
    }

    assembler->checkState(mmt.packetSequenceNumber);

    mmtStream->rapFlag = mmt.rapFlag;
    

    if (mpu.aggregateFlag == 0) {
        DataUnit dataUnit;
        if (!dataUnit.unpack(nstream, mpu.timedFlag, mpu.aggregateFlag)) {
            return;
        }

        if (assembler->assemble(dataUnit.data, mpu.fragmentationIndicator, mmt.packetSequenceNumber)) {
            Common::ReadStream dataStream(assembler->data);
            processMfuData(dataStream);
            assembler->clear();
        }
    }
    else
    {
        while (!nstream.isEof()) {
            DataUnit dataUnit;
            if (!dataUnit.unpack(nstream, mpu.timedFlag, mpu.aggregateFlag)) {
                return;
            }

            if (assembler->assemble(dataUnit.data, mpu.fragmentationIndicator, mmt.packetSequenceNumber)) {
                Common::ReadStream dataStream(assembler->data);
                processMfuData(dataStream);
                assembler->clear();
            }
        }
    }
}

void MmtTlvDemuxer::processMfuData(Common::ReadStream& stream)
{
    std::shared_ptr<MmtStream> mmtStream = getStream(mmt.packetId);
    if (!mmtStream) {
        return;
    }

    if (!mmtStream->mfuDataProcessor) {
        return;
    }

    std::vector<uint8_t> data(stream.leftBytes());
    stream.read(data.data(), stream.leftBytes());

    const auto ret = mmtStream->mfuDataProcessor->process(mmtStream, data);
    if (ret.has_value()) {
        const auto& mfuData = ret.value();
        auto it = std::next(mapStream.begin(), mfuData.streamIndex);
        if (it == mapStream.end()) {
            return;
        }
        
        if(demuxerHandler) {
            switch (mmtStream->assetType) {
            case AssetType::hev1:
                demuxerHandler->onVideoData(it->second, std::make_shared<MfuData>(mfuData));
                break;
            case AssetType::mp4a:
                demuxerHandler->onAudioData(it->second, std::make_shared<MfuData>(mfuData));
                break;
            case AssetType::stpp:
                demuxerHandler->onSubtitleData(it->second, std::make_shared<MfuData>(mfuData));
                break;
            case AssetType::aapp:
                demuxerHandler->onApplicationData(it->second, std::make_shared<MfuData>(mfuData));
                break;
            }
        }
    }
}

void MmtTlvDemuxer::processSignalingMessages(Common::ReadStream& stream)
{
    SignalingMessage signalingMessage;
    if (!signalingMessage.unpack(stream)) {
        return;
    }

    auto assembler = getAssembler(mmt.packetId);
    assembler->checkState(mmt.packetSequenceNumber);

    if (!signalingMessage.aggregationFlag) {
        if (assembler->assemble(signalingMessage.payload, signalingMessage.fragmentationIndicator, mmt.packetSequenceNumber)) {
            Common::ReadStream messageStream(assembler->data);
            processSignalingMessage(messageStream);
            assembler->clear();
        }
    }
    else {
        if (signalingMessage.fragmentationIndicator != FragmentationIndicator::NotFragmented) {
            return;
        }

        Common::ReadStream nstream(signalingMessage.payload);
        while (nstream.isEof()) {
            uint32_t length;
            if (signalingMessage.lengthExtensionFlag)
                length = nstream.getBe32U();
            else
                length = nstream.getBe16U();

            std::vector<uint8_t> message;
            message.resize(length);
            stream.read(message.data(), length);

            if (assembler->assemble(message, signalingMessage.fragmentationIndicator, mmt.packetSequenceNumber)) {
                Common::ReadStream messageStream(assembler->data);
                processSignalingMessage(messageStream);
                assembler->clear();
            }
        }
    }

    return;
}

void MmtTlvDemuxer::processSignalingMessage(Common::ReadStream& stream)
{
    MmtMessageId id = static_cast<MmtMessageId>(stream.peekBe16U());

    switch (id) {
    case MmtMessageId::PaMessage:
        return processPaMessage(stream);
    case MmtMessageId::M2SectionMessage:
        return processM2SectionMessage(stream);
    case MmtMessageId::CaMessage:
        return processCaMessage(stream);
    case MmtMessageId::M2ShortSectionMessage:
        return processM2ShortSectionMessage(stream);
    case MmtMessageId::DataTransmissionMessage:
        return processDataTransmissionMessage(stream);
    }
}

bool MmtTlvDemuxer::isVaildTlv(Common::ReadStream& stream) const
{
    try {
        uint8_t bytes[2];
        stream.peek((char*)bytes, 2);

        // syncByte
        if (bytes[0] != 0x7F) {
            return false;
        }

        // packetType
        if (bytes[1] > 0x04 && bytes[1] < 0xFD) {
            return false;
        }
    }
    catch (const std::runtime_error&) {
        return false;
    }
    return true;
}

}