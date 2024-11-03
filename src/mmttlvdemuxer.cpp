#include "acascard.h"
#include "dataUnit.h"
#include "ecm.h"
#include "m2SectionMessage.h"
#include "m2ShortSectionMessage.h"
#include "mhCdt.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "mhTot.h"
#include "mpuStream.h"
#include "MmtTlvDemuxer.h"
#include "mpt.h"
#include "fragmentAssembler.h"
#include "mpuDataProcessorFactory.h"
#include "nit.h"
#include "paMessage.h"
#include "plt.h"
#include "signalingMessage.h"
#include "smartcard.h"
#include "stream.h"
#include "mmtTableFactory.h"
#include "tlvTableFactory.h"
#include "demuxerHandler.h"

namespace MmtTlv {

MmtTlvDemuxer::MmtTlvDemuxer()
{
    smartCard = std::make_shared<Acas::SmartCard>();
    acasCard = std::make_shared<Acas::AcasCard>(smartCard);
}

bool MmtTlvDemuxer::init()
{
    try {
        smartCard->initCard();
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

int MmtTlvDemuxer::processPacket(Common::StreamBase& stream)
{
    if (stream.leftBytes() < 4) {
        return -1;
    }

    if (!isVaildTlv(stream)) {
        stream.skip(1);
        return -2;
    }

    if (!tlv.unpack(stream)) {
        return -1;
    }

    if (stream.leftBytes() < tlv.getDataLength()) {
        return -1;
    }

    Common::Stream tlvDataStream(tlv.getData());

    if (tlv.getPacketType() == TlvPacketType::TransmissionControlSignalPacket) {
        processTlvTable(tlvDataStream);
        return 1;
    }

    if (tlv.getPacketType() != TlvPacketType::HeaderCompressedIpPacket) {
        return 1;
    }

    compressedIPPacket.unpack(tlvDataStream);
    mmt.unpack(tlvDataStream);

    if (mmt.extensionHeaderScrambling.has_value()) {
        if (mmt.extensionHeaderScrambling->encryptionFlag == EncryptionFlag::ODD ||
            mmt.extensionHeaderScrambling->encryptionFlag == EncryptionFlag::EVEN) {
            if (!acasCard->ready) {
                return 1;
            }
            else {
                mmt.decryptPayload(&acasCard->lastDecryptedEcm);
            }
        }
    }

    Common::Stream mmtpPayloadStream(mmt.payload);
    switch (mmt.payloadType) {
    case PayloadType::Mpu:
        processMpu(mmtpPayloadStream);
        break;
    case PayloadType::ContainsOneOrMoreControlMessage:
        processSignalingMessages(mmtpPayloadStream);
        break;
    }

    return 1;
}

void MmtTlvDemuxer::processPaMessage(Common::Stream& stream)
{
    PaMessage message;
    message.unpack(stream);

    Common::Stream nstream(message.table);
    while (!nstream.isEof()) {
        processMmtTable(nstream);
    }
}

void MmtTlvDemuxer::processM2SectionMessage(Common::Stream& stream)
{
    M2SectionMessage message;
    message.unpack(stream);
    processMmtTable(stream);
}

void MmtTlvDemuxer::processM2ShortSectionMessage(Common::Stream& stream)
{
    M2ShortSectionMessage message;
    message.unpack(stream);
    processMmtTable(stream);
}

void MmtTlvDemuxer::processTlvTable(Common::Stream& stream)
{
    uint8_t tableId = stream.peek8U();
    const auto table = TlvTableFactory::create(tableId);
    if (table == nullptr) {
        return;
    }

    table->unpack(stream);

    switch (tableId) {
    case MmtTableId::Ecm:
        if (demuxerHandler) {
            demuxerHandler->onNit(std::dynamic_pointer_cast<Nit>(table));
        }
        break;
    }
}

void MmtTlvDemuxer::processMmtTable(Common::Stream& stream)
{
    uint8_t tableId = stream.peek8U();
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
    case MmtTableId::Ecm:
    {
        processEcm(std::dynamic_pointer_cast<Ecm>(table));
        break;
    }
    }

    switch (tableId) {
    case MmtTableId::Ecm:
        if (demuxerHandler) {
            demuxerHandler->onEcm(std::dynamic_pointer_cast<Ecm>(table));
        }
        break;
    case MmtTableId::MhCdt:
        if (demuxerHandler) {
            demuxerHandler->onMhCdt(std::dynamic_pointer_cast<MhCdt>(table));
        }
        break;
    case MmtTableId::MhEit:
        if (demuxerHandler) {
            demuxerHandler->onMhEit(std::dynamic_pointer_cast<MhEit>(table));
        }
        break;
    case MmtTableId::MhSdt:
        if (demuxerHandler) {
            demuxerHandler->onMhSdt(std::dynamic_pointer_cast<MhSdt>(table));
        }
        break;
    case MmtTableId::MhTot:
        if (demuxerHandler) {
            demuxerHandler->onMhTot(std::dynamic_pointer_cast<MhTot>(table));
        }
        break;
    case MmtTableId::Mpt:
        if (demuxerHandler) {
            demuxerHandler->onMpt(std::dynamic_pointer_cast<Mpt>(table));
        }
        break;
    case MmtTableId::Plt:
        if (demuxerHandler) {
            demuxerHandler->onPlt(std::dynamic_pointer_cast<Plt>(table));
        }
        break;
    }
}

void MmtTlvDemuxer::processMmtPackageTable(const std::shared_ptr<Mpt>& mpt)
{
    for (auto& asset : mpt->assets) {
        std::shared_ptr<MpuStream> mpuStream;

        for (auto& locationInfo : asset.locationInfos) {
            if (locationInfo.locationType == 0) {
                if (asset.assetType == makeAssetType('h', 'e', 'v', '1') ||
                    asset.assetType == makeAssetType('m', 'p', '4', 'a') ||
                    asset.assetType == makeAssetType('s', 't', 'p', 'p')) {
                    mpuStream = getStream(locationInfo.packetId, true);
                    mpuStream->assetType = asset.assetType;

                    if (!mpuStream->mpuDataProcessor) {
                        mpuStream->mpuDataProcessor = MpuDataProcessorFactory::create(mpuStream->assetType);
                    }
                }
            }
        }

        if (!mpuStream) {
            continue;
        }

        for (auto& descriptor : asset.descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MpuTimestampDescriptor::kDescriptorTag:
                processMpuTimestampDescriptor(std::dynamic_pointer_cast<MpuTimestampDescriptor>(descriptor), mpuStream);
                break;
            case MpuExtendedTimestampDescriptor::kDescriptorTag:
                processMpuExtendedTimestampDescriptor(std::dynamic_pointer_cast<MpuExtendedTimestampDescriptor>(descriptor), mpuStream);
                break;
            }
        }
    }
}

void MmtTlvDemuxer::processMpuTimestampDescriptor(const std::shared_ptr<MpuTimestampDescriptor>& descriptor, std::shared_ptr<MpuStream> mpuStream)
{
    for (auto& ts : descriptor->entries) {
        bool find = false;
        for (int i = 0; i < mpuStream->mpuTimestamps.size(); i++) {
            if (mpuStream->mpuTimestamps[i].mpuSequenceNumber == ts.mpuSequenceNumber) {
                mpuStream->mpuTimestamps[i].mpuPresentationTime = ts.mpuPresentationTime;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        for (int i = 0; i < mpuStream->mpuTimestamps.size(); i++) {
            if (mpuStream->mpuTimestamps[i].mpuSequenceNumber < mpuStream->lastMpuSequenceNumber) {
                mpuStream->mpuTimestamps[i].mpuSequenceNumber = ts.mpuSequenceNumber;
                mpuStream->mpuTimestamps[i].mpuPresentationTime = ts.mpuPresentationTime;
                find = true;
                break;
            }
        }

        if (find) {
            continue;
        }

        if (mpuStream->mpuTimestamps.size() >= 100) {
            int minIndex = 0;
            uint32_t minMpuSequenceNumber = 0xFFFFFFFF;
            for (int i = 0; i < mpuStream->mpuTimestamps.size(); i++) {
                if (minMpuSequenceNumber > mpuStream->mpuTimestamps[i].mpuSequenceNumber) {
                    minIndex = i;
                    minMpuSequenceNumber = mpuStream->mpuTimestamps[i].mpuSequenceNumber;
                }
            }

            mpuStream->mpuTimestamps[minIndex].mpuSequenceNumber = ts.mpuSequenceNumber;
            mpuStream->mpuTimestamps[minIndex].mpuPresentationTime = ts.mpuPresentationTime;
        }
        else {
            mpuStream->mpuTimestamps.push_back(ts);
        }
    }
}

void MmtTlvDemuxer::processMpuExtendedTimestampDescriptor(const std::shared_ptr<MpuExtendedTimestampDescriptor>& descriptor, std::shared_ptr<MpuStream> mpuStream)
{
    if (descriptor->timescaleFlag) {
        mpuStream->timeBase.num = 1;
        mpuStream->timeBase.den = descriptor->timescale;
    }

    for (auto& ts : descriptor->entries) {
        if (mpuStream->lastMpuSequenceNumber > ts.mpuSequenceNumber)
            continue;

        bool find = false;
        for (int i = 0; i < mpuStream->mpuExtendedTimestamps.size(); i++) {
            if (mpuStream->mpuExtendedTimestamps[i].mpuSequenceNumber == ts.mpuSequenceNumber) {
                mpuStream->mpuExtendedTimestamps[i].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
                mpuStream->mpuExtendedTimestamps[i].numOfAu = ts.numOfAu;
                mpuStream->mpuExtendedTimestamps[i].ptsOffsets = ts.ptsOffsets;
                mpuStream->mpuExtendedTimestamps[i].dtsPtsOffsets = ts.dtsPtsOffsets;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        for (int i = 0; i < mpuStream->mpuExtendedTimestamps.size(); i++) {
            if (mpuStream->mpuExtendedTimestamps[i].mpuSequenceNumber < mpuStream->lastMpuSequenceNumber) {
                mpuStream->mpuExtendedTimestamps[i].mpuSequenceNumber = ts.mpuSequenceNumber;
                mpuStream->mpuExtendedTimestamps[i].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
                mpuStream->mpuExtendedTimestamps[i].numOfAu = ts.numOfAu;
                mpuStream->mpuExtendedTimestamps[i].ptsOffsets = ts.ptsOffsets;
                mpuStream->mpuExtendedTimestamps[i].dtsPtsOffsets = ts.dtsPtsOffsets;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        if (mpuStream->mpuExtendedTimestamps.size() >= 100) {
            int minIndex = 0;
            uint32_t minMpuSequenceNumber = 0xFFFFFFFF;
            for (int i = 0; i < mpuStream->mpuExtendedTimestamps.size(); i++) {
                if (minMpuSequenceNumber > mpuStream->mpuExtendedTimestamps[i].mpuSequenceNumber) {
                    minIndex = i;
                    minMpuSequenceNumber = mpuStream->mpuExtendedTimestamps[i].mpuSequenceNumber;
                }
            }

            mpuStream->mpuExtendedTimestamps[minIndex].mpuSequenceNumber = ts.mpuSequenceNumber;
            mpuStream->mpuExtendedTimestamps[minIndex].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
            mpuStream->mpuExtendedTimestamps[minIndex].numOfAu = ts.numOfAu;
            mpuStream->mpuExtendedTimestamps[minIndex].ptsOffsets = ts.ptsOffsets;
            mpuStream->mpuExtendedTimestamps[minIndex].dtsPtsOffsets = ts.dtsPtsOffsets;
        }
        else {
            mpuStream->mpuExtendedTimestamps.push_back(ts);
        }
    }
}

void MmtTlvDemuxer::processEcm(std::shared_ptr<Ecm> ecm)
{
    try {
        acasCard->decryptEcm(ecm->ecmData);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
}

void MmtTlvDemuxer::clear()
{
    mapAssembler.clear();
    mpuData.clear();
    mapStream.clear();
    streamIndex = 0;

    if (acasCard) {
        acasCard->clear();
    }
}

std::shared_ptr<FragmentAssembler> MmtTlvDemuxer::getAssembler(uint16_t pid)
{
    if (mapAssembler.find(pid) == mapAssembler.end()) {
        auto assembler = std::make_shared<FragmentAssembler>();
        mapAssembler[pid] = assembler;
        return assembler;
    }
    else {
        return mapAssembler[pid];
    }
}

std::shared_ptr<MpuStream> MmtTlvDemuxer::getStream(uint16_t pid, bool create)
{
    if (mapStream.find(pid) == mapStream.end()) {
        if (create) {
            auto stream = std::make_shared<MpuStream>();
            stream->streamIndex = streamIndex++;
            stream->pid = pid;
            mapStream[pid] = stream;
            return stream;
        }
        else {
            return nullptr;
        }
    }
    else {
        return mapStream[pid];
    }
}

std::shared_ptr<MpuStream> MmtTlvDemuxer::getStream(uint16_t pid)
{
    return getStream(pid, false);
}

void MmtTlvDemuxer::processMpu(Common::Stream& stream)
{
    if (!mpu.unpack(stream))
        return;

    auto assembler = getAssembler(mmt.packetId);
    Common::Stream nstream(mpu.payload);
    std::shared_ptr<MpuStream> mpuStream = getStream(mmt.packetId);

    if (!mpuStream) {
        return;
    }

    if (mpu.aggregateFlag && mpu.fragmentationIndicator != FragmentationIndicator::NotFragmented) {
        return;
    }

    if (mpu.fragmentType != FragmentType::Mfu) {
        return;
    }

    if (assembler->state == FragmentAssembler::State::Init && !mmt.rapFlag) {
        return;
    }

    if (assembler->state == FragmentAssembler::State::Init) {
        mpuStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
    }
    else if (mpu.mpuSequenceNumber == mpuStream->lastMpuSequenceNumber + 1) {
        mpuStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
        mpuStream->auIndex = 0;
    }
    else if (mpu.mpuSequenceNumber != mpuStream->lastMpuSequenceNumber) {
        std::cerr << "drop found (" << mpuStream->lastMpuSequenceNumber << " != " << mpu.mpuSequenceNumber << ")" << std::endl;
        assembler->state = FragmentAssembler::State::Init;
        return;
    }

    assembler->checkState(mmt.packetSequenceNumber);

    if (mmt.rapFlag) {
        mpuStream->flags |= AV_PKT_FLAG_KEY;
    }

    if (mpu.aggregateFlag == 0) {
        DataUnit dataUnit;
        if (!dataUnit.unpack(nstream, mpu.timedFlag, mpu.aggregateFlag)) {
            return;
        }

        if (assembler->assemble(dataUnit.data, mpu.fragmentationIndicator, mmt.packetSequenceNumber)) {
            Common::Stream dataStream(assembler->data);
            processMpuData(dataStream);
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
                Common::Stream dataStream(assembler->data);
                processMpuData(dataStream);
                assembler->clear();
            }
        }
    }
}

void MmtTlvDemuxer::processMpuData(Common::Stream& stream)
{
    std::shared_ptr<MpuStream> mpuStream = getStream(mmt.packetId);
    if (!mpuStream) {
        return;
    }

    if (!mpuStream->mpuDataProcessor) {
        return;
    }

    std::vector<uint8_t> data(stream.leftBytes());
    stream.read(data.data(), stream.leftBytes());

    const auto ret = mpuStream->mpuDataProcessor->process(mpuStream, data);
    if (ret.has_value()) {
        const auto& mpuData = ret.value();
        auto it = std::next(mapStream.begin(), mpuData.streamIndex);
        if (it == mapStream.end()) {
            return;
        }

        switch (mpuStream->assetType) {
        case makeAssetType('h', 'e', 'v', '1'):
            if(demuxerHandler) {
                demuxerHandler->onVideoData(it->second, std::make_shared<MpuData>(mpuData));
            }
            break;
        case makeAssetType('m', 'p', '4', 'a'):
            if(demuxerHandler) {
                demuxerHandler->onAudioData(it->second, std::make_shared<MpuData>(mpuData));
            }
            break;
        case makeAssetType('s', 't', 'p', 'p'):
            if(demuxerHandler) {
                demuxerHandler->onSubtitleData(it->second, std::make_shared<MpuData>(mpuData));
            }
            break;
        }
    }
}

void MmtTlvDemuxer::processSignalingMessages(Common::Stream& stream)
{
    SignalingMessage signalingMessage;

    if (!signalingMessage.unpack(stream)) {
        return;
    }

    auto assembler = getAssembler(mmt.packetId);
    assembler->checkState(mmt.packetSequenceNumber);

    if (!signalingMessage.aggregationFlag) {
        if (assembler->assemble(signalingMessage.payload, signalingMessage.fragmentationIndicator, mmt.packetSequenceNumber)) {
            Common::Stream messageStream(assembler->data);
            processSignalingMessage(messageStream);
            assembler->clear();
        }
    }
    else {
        if (signalingMessage.fragmentationIndicator != FragmentationIndicator::NotFragmented) {
            return;
        }

        Common::Stream nstream(signalingMessage.payload);
        while (nstream.isEof()) {
            uint32_t length;

            if (signalingMessage.lengthExtensionFlag)
                length = nstream.getBe32U();
            else
                length = nstream.getBe16U();

            if (assembler->assemble(signalingMessage.payload, signalingMessage.fragmentationIndicator, mmt.packetSequenceNumber)) {
                Common::Stream messageStream(assembler->data);
                processSignalingMessage(messageStream);
                assembler->clear();
            }
        }
    }

    return;
}

void MmtTlvDemuxer::processSignalingMessage(Common::Stream& stream)
{
    MmtMessageId id = static_cast<MmtMessageId>(stream.peekBe16U());

    switch (id) {
    case MmtMessageId::PaMessage:
        return processPaMessage(stream);
    case MmtMessageId::M2SectionMessage:
        return processM2SectionMessage(stream);
    case MmtMessageId::M2ShortSectionMessage:
        return processM2ShortSectionMessage(stream);
        break;
    }
}

bool MmtTlvDemuxer::isVaildTlv(Common::StreamBase& stream) const
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