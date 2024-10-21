#include "MmtTlvDemuxer.h"
#include "stream.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "plt.h"
#include "tlvNit.h"
#include "mpt.h"
#include "mhTot.h"
#include "ecm.h"
#include "smartcard.h"
#include "acascard.h"
#include "mpuDataProcessorFactory.h"
#include "mpuAssembler.h"
#include "mhCdt.h"

MmtTlvDemuxer::MmtTlvDemuxer()
{
    smartCard = std::make_shared<SmartCard>();
    acasCard = std::make_shared<AcasCard>(smartCard);
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

int MmtTlvDemuxer::processPacket(StreamBase& stream)
{
    mpuDatas.clear();
    tables.clear();
    tlvTables.clear();

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

    if (stream.leftBytes() < tlv.dataLength) {
        return -1;
    }

    Stream tlvDataStream(tlv.getData());

    if (tlv.packetType == TLV_PACKET_TYPE::TLV_TRANSMISSION_CONTROL_SIGNAL_PACKET) {
        uint16_t tableId = tlvDataStream.peek8U();
        switch (tableId) {
        case 0x40:
        {
            std::shared_ptr<TlvNit> tlvNit = std::make_shared<TlvNit>();
            tlvNit->unpack(tlvDataStream);
            tlvTables.push_back(tlvNit);
            break;
        }
        }

        return 1;
    }

    if (tlv.packetType != TLV_PACKET_TYPE::TLV_HEADER_COMPRESSED_IP_PACKET) {
        return 1;
    }

    compressedIPPacket.unpack(tlvDataStream);
    mmtp.unpack(tlvDataStream);

    if (mmtp.hasExtensionHeaderScrambling) {
        if (mmtp.extensionHeaderScrambling.encryptionFlag == ENCRYPTION_FLAG::ODD ||
            mmtp.extensionHeaderScrambling.encryptionFlag == ENCRYPTION_FLAG::EVEN) {
            if (!acasCard->ready) {
                return 1;
            }
            else {
                mmtp.decryptPayload(&acasCard->lastDecryptedEcm);
            }
        }
    }

    Stream mmtpPayloadStream(mmtp.payload);
    switch (mmtp.payloadType) {
    case PAYLOAD_TYPE::MPU:
        processMpu(mmtpPayloadStream);
        break;
    case PAYLOAD_TYPE::CONTAINS_ONE_OR_MORE_CONTROL_MESSAGE:
        processSignalingMessages(mmtpPayloadStream);
        break;
    }

    return 1;
}

void MmtTlvDemuxer::processPaMessage(Stream& stream)
{
    PaMessage message;
    message.unpack(stream);

    Stream nstream(message.table);
    while (!nstream.isEOF()) {
        processTable(nstream);
    }
}

void MmtTlvDemuxer::processM2SectionMessage(Stream& stream)
{
    M2SectionMessage message;
    message.unpack(stream);
    processTable(stream);
}

void MmtTlvDemuxer::processM2ShortSectionMessage(Stream& stream)
{
    M2ShortSectionMessage message;
    message.unpack(stream);
    processTable(stream);
}

void MmtTlvDemuxer::processTable(Stream& stream)
{
    uint8_t tableId = stream.peek8U();
    switch (tableId) {
    case MMT_TABLE_ID::MPT:
    {
        std::shared_ptr<Mpt> mpt = std::make_shared<Mpt>();
        mpt->unpack(stream);
        tables.push_back(mpt);

        processMmtPackageTable(mpt);
        break;
    }
    case MMT_TABLE_ID::PLT:
    {
        std::shared_ptr<Plt> plt = std::make_shared<Plt>();
        plt->unpack(stream);
        tables.push_back(plt);
        break;
    }
    case MMT_TABLE_ID::MH_EIT:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9A:
    case 0x9B:
    {
        std::shared_ptr<MhEit> mhEit = std::make_shared<MhEit>();
        mhEit->unpack(stream);
        tables.push_back(mhEit);
        break;
    }
    case MMT_TABLE_ID::MH_SDT:
    {
        std::shared_ptr<MhSdt> mhSdt = std::make_shared<MhSdt>();
        mhSdt->unpack(stream);
        tables.push_back(mhSdt);
        break;
    }
    case MMT_TABLE_ID::MH_TOT:
    {
        std::shared_ptr<MhTot> mhTot = std::make_shared<MhTot>();
        mhTot->unpack(stream);
        tables.push_back(mhTot);
        break;
    }
    case MMT_TABLE_ID::ECM:
    {
        std::shared_ptr<Ecm> ecm = std::make_shared<Ecm>();
        ecm->unpack(stream);
        tables.push_back(ecm);

        processEcm(ecm);
        break;
    }
    case MMT_TABLE_ID::MH_CDT:
    {
        std::shared_ptr<MhCdt> mhCdt = std::make_shared<MhCdt>();
        mhCdt->unpack(stream);
        tables.push_back(mhCdt);
        break;
    }
    }

    stream.skip(stream.leftBytes());
}

void MmtTlvDemuxer::processMmtPackageTable(const std::shared_ptr<Mpt>& mpt)
{
    for (auto& asset : mpt->assets) {
        std::shared_ptr<MmtStream> mmtStream;

        for (auto& locationInfo : asset.locationInfos) {
            if (locationInfo.locationType == 0) {
                switch (asset.assetType) {
                case makeAssetType('h', 'e', 'v', '1'):
                    mmtStream = getStream(locationInfo.packetId, true);
                    break;
                case makeAssetType('m', 'p', '4', 'a'):
                    mmtStream = getStream(locationInfo.packetId, true);
                    break;
                case makeAssetType('s', 't', 'p', 'p'):
                    mmtStream = getStream(locationInfo.packetId, true);
                    break;
                }

                if (mmtStream) {
                    mmtStream->assetType = asset.assetType;
                    if (!mmtStream->mpuDataProcessor) {
                        mmtStream->mpuDataProcessor = MpuDataProcessorFactory::create(asset.assetType);
                    }
                }
            }
        }

        if (!mmtStream) {
            continue;
        }

        for (auto& descriptor : asset.descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MpuTimestampDescriptor::kDescriptorTag:
                processMpuTimestampDescriptor(std::dynamic_pointer_cast<MpuTimestampDescriptor>(descriptor), mmtStream);
                break;
            case MpuExtendedTimestampDescriptor::kDescriptorTag:
                processMpuExtendedTimestampDescriptor(std::dynamic_pointer_cast<MpuExtendedTimestampDescriptor>(descriptor), mmtStream);
                break;
            }
        }
    }
}

void MmtTlvDemuxer::processMpuTimestampDescriptor(const std::shared_ptr<MpuTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream> mmtStream)
{
    for (auto& ts : descriptor->entries) {
        bool find = false;
        for (int i = 0; i < mmtStream->mpuTimestamps.size(); i++) {
            if (mmtStream->mpuTimestamps[i].mpuSequenceNumber == ts.mpuSequenceNumber) {
                mmtStream->mpuTimestamps[i].mpuPresentationTime = ts.mpuPresentationTime;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        for (int i = 0; i < mmtStream->mpuTimestamps.size(); i++) {
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
            int minIndex = 0;
            uint32_t minMpuSequenceNumber = 0xFFFFFFFF;
            for (int i = 0; i < mmtStream->mpuTimestamps.size(); i++) {
                if (minMpuSequenceNumber > mmtStream->mpuTimestamps[i].mpuSequenceNumber) {
                    minIndex = i;
                    minMpuSequenceNumber = mmtStream->mpuTimestamps[i].mpuSequenceNumber;
                }
            }

            mmtStream->mpuTimestamps[minIndex].mpuSequenceNumber = ts.mpuSequenceNumber;
            mmtStream->mpuTimestamps[minIndex].mpuPresentationTime = ts.mpuPresentationTime;
        }
        else {
            mmtStream->mpuTimestamps.push_back(ts);
        }
    }
}

void MmtTlvDemuxer::processMpuExtendedTimestampDescriptor(const std::shared_ptr<MpuExtendedTimestampDescriptor>& descriptor, std::shared_ptr<MmtStream> mmtStream)
{
    if (descriptor->timescaleFlag) {
        mmtStream->timeBase.num = 1;
        mmtStream->timeBase.den = descriptor->timescale;
    }

    for (auto& ts : descriptor->entries) {
        if (mmtStream->lastMpuSequenceNumber > ts.mpuSequenceNumber)
            continue;

        bool find = false;
        for (int i = 0; i < mmtStream->mpuExtendedTimestamps.size(); i++) {
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

        for (int i = 0; i < mmtStream->mpuExtendedTimestamps.size(); i++) {
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
            int minIndex = 0;
            uint32_t minMpuSequenceNumber = 0xFFFFFFFF;
            for (int i = 0; i < mmtStream->mpuExtendedTimestamps.size(); i++) {
                if (minMpuSequenceNumber > mmtStream->mpuExtendedTimestamps[i].mpuSequenceNumber) {
                    minIndex = i;
                    minMpuSequenceNumber = mmtStream->mpuExtendedTimestamps[i].mpuSequenceNumber;
                }
            }

            mmtStream->mpuExtendedTimestamps[minIndex].mpuSequenceNumber = ts.mpuSequenceNumber;
            mmtStream->mpuExtendedTimestamps[minIndex].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
            mmtStream->mpuExtendedTimestamps[minIndex].numOfAu = ts.numOfAu;
            mmtStream->mpuExtendedTimestamps[minIndex].ptsOffsets = ts.ptsOffsets;
            mmtStream->mpuExtendedTimestamps[minIndex].dtsPtsOffsets = ts.dtsPtsOffsets;
        }
        else {
            mmtStream->mpuExtendedTimestamps.push_back(ts);
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
    mpuDatas.clear();
    tables.clear();
    tlvTables.clear();

    mapAssembler.clear();
    mpuData.clear();
    mapStream.clear();
    streamIndex = 0;

    if (acasCard) {
        acasCard->clear();
    }
}

std::shared_ptr<MpuAssembler> MmtTlvDemuxer::getAssembler(uint16_t pid)
{
    if (mapAssembler.find(pid) == mapAssembler.end()) {
        auto assembler = std::make_shared<MpuAssembler>();
        mapAssembler[pid] = assembler;
        return assembler;
    }
    else {
        return mapAssembler[pid];
    }
}

std::shared_ptr<MmtStream> MmtTlvDemuxer::getStream(uint16_t pid, bool create)
{
    if (mapStream.find(pid) == mapStream.end()) {
        if (create) {
            auto stream = std::make_shared<MmtStream>();
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

void MmtTlvDemuxer::processMpu(Stream& stream)
{
    if (!mpu.unpack(stream))
        return;

    auto assembler = getAssembler(mmtp.packetId);
    Stream nstream(mpu.payload);
    std::shared_ptr<MmtStream> mmtStream = getStream(mmtp.packetId);

    if (!mmtStream) {
        return;
    }

    if (mpu.aggregateFlag && mpu.fragmentationIndicator != NOT_FRAGMENTED) {
        return;
    }

    if (mpu.fragmentType != MPU_FRAGMENT_TYPE::MPU) {
        return;
    }

    if (assembler->state == MPU_ASSEMBLER_STATE::INIT && !mmtp.rapFlag) {
        return;
    }

    if (assembler->state == MPU_ASSEMBLER_STATE::INIT) {
        mmtStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
    }
    else if (mpu.mpuSequenceNumber == mmtStream->lastMpuSequenceNumber + 1) {
        mmtStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
        mmtStream->auIndex = 0;
    }
    else if (mpu.mpuSequenceNumber != mmtStream->lastMpuSequenceNumber) {
        std::cerr << "drop found (" << mmtStream->lastMpuSequenceNumber << " != " << mpu.mpuSequenceNumber << ")" << std::endl;
        assembler->state = MPU_ASSEMBLER_STATE::INIT;
        return;
    }

    assembler->checkState(mmtp.packetSequenceNumber);

    if (mmtp.rapFlag) {
        mmtStream->flags |= AV_PKT_FLAG_KEY;
    }

    if (mpu.aggregateFlag == 0) {
        DataUnit dataUnit;
        if (!dataUnit.unpack(nstream, mpu.timedFlag, mpu.aggregateFlag)) {
            return;
        }

        if (assembler->assemble(dataUnit.data, mpu.fragmentationIndicator, mmtp.packetSequenceNumber)) {
            Stream dataStream(assembler->data);
            processMpuData(dataStream);
            assembler->clear();
        }
    }
    else
    {
        while (!nstream.isEOF()) {
            DataUnit dataUnit;
            if (!dataUnit.unpack(nstream, mpu.timedFlag, mpu.aggregateFlag)) {
                return;
            }

            if (assembler->assemble(dataUnit.data, mpu.fragmentationIndicator, mmtp.packetSequenceNumber)) {
                Stream dataStream(assembler->data);
                processMpuData(dataStream);
                assembler->clear();
            }
        }
    }
}

void MmtTlvDemuxer::processMpuData(Stream& stream)
{
    std::shared_ptr<MmtStream> mmtStream = getStream(mmtp.packetId);
    if (!mmtStream) {
        return;
    }

    if (!mmtStream->mpuDataProcessor) {
        return;
    }

    std::vector<uint8_t> data(stream.leftBytes());
    stream.read(data.data(), stream.leftBytes());

    const auto ret = mmtStream->mpuDataProcessor->process(mmtStream, data);
    if (ret.has_value()) {
        mpuDatas.push_back(std::make_shared<MpuData>(ret.value()));
    }
}

void MmtTlvDemuxer::processSignalingMessages(Stream& stream)
{
    SignalingMessage signalingMessage;

    auto assembler = getAssembler(mmtp.packetId);

    if (!signalingMessage.unpack(stream)) {
        return;
    }

    assembler->checkState(mmtp.packetSequenceNumber);

    if (!signalingMessage.aggregationFlag) {
        if (assembler->assemble(signalingMessage.payload, signalingMessage.fragmentationIndicator, mmtp.packetSequenceNumber)) {
            Stream messageStream(assembler->data);
            processSignalingMessage(messageStream);
            assembler->clear();
        }
    }
    else {
        if (signalingMessage.fragmentationIndicator != NOT_FRAGMENTED) {
            return;
        }

        Stream nstream(signalingMessage.payload);
        while (nstream.isEOF()) {
            uint32_t length;

            if (signalingMessage.lengthExtensionFlag)
                length = nstream.getBe32U();
            else
                length = nstream.getBe16U();

            if (assembler->assemble(signalingMessage.payload, signalingMessage.fragmentationIndicator, mmtp.packetSequenceNumber)) {
                Stream messageStream(assembler->data);
                processSignalingMessage(messageStream);
                assembler->clear();
            }
        }
    }

    return;
}

void MmtTlvDemuxer::processSignalingMessage(Stream& stream)
{
    uint16_t type = stream.peekBe16U();
    switch (type) {
    case PA_MESSAGE_ID:
        return processPaMessage(stream);
    case M2_SECTION_MESSAGE:
        return processM2SectionMessage(stream);
    case M2_SHORT_SECTION_MESSAGE:
        return processM2ShortSectionMessage(stream);
        break;
    }
}

bool MmtTlvDemuxer::isVaildTlv(StreamBase& stream) const
{
    try {
        uint8_t bytes[2];
        stream.peek((char*)bytes, 2);

        //syncByte
        if (bytes[0] != 0x7F) {
            return false;
        }

        //packetType
        if (bytes[1] > 0x04 && bytes[1] < 0xFD) {
            return false;
        }
    }
    catch (const std::runtime_error&) {
        return false;
    }
    return true;
}
