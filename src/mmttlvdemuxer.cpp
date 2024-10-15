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

bool MmtTlvDemuxer::init()
{
    smartCard = new SmartCard();
    acasCard = new AcasCard(smartCard);

    try {
        smartCard->initializeCard();
        smartCard->connect();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }

    return true;
}

int MmtTlvDemuxer::processPacket(Stream& stream)
{
    avpackets.clear();
    clearTables();

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
            TlvNit* tlvNit = new TlvNit();
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
                return true;
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
        processMmtPackageTable(stream);
        break;
    }
    case MMT_TABLE_ID::PLT:
    {
        Plt* plt = new Plt();
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
        MhEit* mhEit = new MhEit();
        mhEit->unpack(stream);
        tables.push_back(mhEit);
        break;
    }
    case MMT_TABLE_ID::MH_SDT:
    {
        MhSdt* mhSdt = new MhSdt();
        mhSdt->unpack(stream);
        tables.push_back(mhSdt);
        break;
    }
    case MMT_TABLE_ID::MH_TOT:
    {
        MhTot* mhTot = new MhTot();
        mhTot->unpack(stream);
        tables.push_back(mhTot);
        break;
    }
    case MMT_TABLE_ID::ECM:
    {
        Ecm* ecm = new Ecm();
        ecm->unpack(stream);
        tables.push_back(ecm);
        processEcm(ecm);
        break;
    }
    }

    stream.skip(stream.leftBytes());
}

void MmtTlvDemuxer::processMmtPackageTable(Stream& stream)
{
    Mpt* mpt = new Mpt();
    mpt->unpack(stream);

    for (auto& asset : mpt->assets) {
        MmtpStream* mmtpStream = nullptr;

        for (auto& locationInfo : asset.locationInfos) {
            if (locationInfo.locationType == 0) {
                switch (asset.assetType) {
                case makeTag('h', 'e', 'v', '1'):
                    mmtpStream = getStream(locationInfo.packetId, true);
                    mmtpStream->codecType = AVMEDIA_TYPE_VIDEO;
                    mmtpStream->codecId = AV_CODEC_ID_HEVC;
                    break;
                case makeTag('m', 'p', '4', 'a'):
                    mmtpStream = getStream(locationInfo.packetId, true);
                    mmtpStream->codecType = AVMEDIA_TYPE_AUDIO;
                    mmtpStream->codecId = AV_CODEC_ID_AAC_LATM;
                    break;
                case makeTag('s', 't', 'p', 'p'):
                    mmtpStream = getStream(locationInfo.packetId, true);
                    mmtpStream->codecType = AVMEDIA_TYPE_SUBTITLE;
                    mmtpStream->codecId = AV_CODEC_ID_TTML;
                    break;
                }
            }
        }

        if (mmtpStream) {
            Stream nstream(asset.assetDescriptorsByte);
            while (!nstream.isEOF()) {
                processMptDescriptor(nstream, mmtpStream);
            }
        }
    }

    tables.push_back(mpt);
}

void MmtTlvDemuxer::processMptDescriptor(Stream& stream, MmtpStream* mmtpStream)
{
    uint16_t descriptorId = stream.peekBe16U();
    switch (descriptorId) {
    case MpuTimestampDescriptor::kDescriptorTag:
        return processMpuTimestampDescriptor(stream, mmtpStream);
    case MpuExtendedTimestampDescriptor::kDescriptorTag:
        return processMpuExtendedTimestampDescriptor(stream, mmtpStream);
    default:
        stream.skip(2); //descriptorId
        uint16_t descriptorLength = stream.get8U();
        stream.skip(descriptorLength);
        return;
    }
}

void MmtTlvDemuxer::processMpuTimestampDescriptor(Stream& stream, MmtpStream* mmtpStream)
{
    MpuTimestampDescriptor descriptor;
    descriptor.unpack(stream);

    for (auto& ts : descriptor.entries) {
        bool find = false;
        for (int i = 0; i < mmtpStream->mpuTimestamps.size(); i++) {
            if (mmtpStream->mpuTimestamps[i].mpuSequenceNumber == ts.mpuSequenceNumber) {
                mmtpStream->mpuTimestamps[i].mpuPresentationTime = ts.mpuPresentationTime;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        for (int i = 0; i < mmtpStream->mpuTimestamps.size(); i++) {
            if (mmtpStream->mpuTimestamps[i].mpuSequenceNumber < mmtpStream->lastMpuSequenceNumber) {
                mmtpStream->mpuTimestamps[i].mpuSequenceNumber = ts.mpuSequenceNumber;
                mmtpStream->mpuTimestamps[i].mpuPresentationTime = ts.mpuPresentationTime;
                find = true;
                break;
            }
        }

        if (find) {
            continue;
        }

        if (mmtpStream->mpuTimestamps.size() >= 100) {
            int minIndex = 0;
            uint32_t minMpuSequenceNumber = 0xFFFFFFFF;
            for (int i = 0; i < mmtpStream->mpuTimestamps.size(); i++) {
                if (minMpuSequenceNumber > mmtpStream->mpuTimestamps[i].mpuSequenceNumber) {
                    minIndex = i;
                    minMpuSequenceNumber = mmtpStream->mpuTimestamps[i].mpuSequenceNumber;
                }
            }

            mmtpStream->mpuTimestamps[minIndex].mpuSequenceNumber = ts.mpuSequenceNumber;
            mmtpStream->mpuTimestamps[minIndex].mpuPresentationTime = ts.mpuPresentationTime;
        }
        else {
            mmtpStream->mpuTimestamps.push_back(ts);
        }
    }
}

void MmtTlvDemuxer::processMpuExtendedTimestampDescriptor(Stream& stream, MmtpStream* mmtpStream)
{
    MpuExtendedTimestampDescriptor descriptor;
    descriptor.unpack(stream);

    if (descriptor.timescaleFlag) {
        mmtpStream->timeBase.num = 1;
        mmtpStream->timeBase.den = descriptor.timescale;
    }

    for (auto& ts : descriptor.entries) {
        if (mmtpStream->lastMpuSequenceNumber > ts.mpuSequenceNumber)
            continue;

        bool find = false;
        for (int i = 0; i < mmtpStream->mpuExtendedTimestamps.size(); i++) {
            if (mmtpStream->mpuExtendedTimestamps[i].mpuSequenceNumber == ts.mpuSequenceNumber) {
                mmtpStream->mpuExtendedTimestamps[i].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
                mmtpStream->mpuExtendedTimestamps[i].numOfAu = ts.numOfAu;
                mmtpStream->mpuExtendedTimestamps[i].ptsOffsets = ts.ptsOffsets;
                mmtpStream->mpuExtendedTimestamps[i].dtsPtsOffsets = ts.dtsPtsOffsets;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        for (int i = 0; i < mmtpStream->mpuExtendedTimestamps.size(); i++) {
            if (mmtpStream->mpuExtendedTimestamps[i].mpuSequenceNumber < mmtpStream->lastMpuSequenceNumber) {
                mmtpStream->mpuExtendedTimestamps[i].mpuSequenceNumber = ts.mpuSequenceNumber;
                mmtpStream->mpuExtendedTimestamps[i].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
                mmtpStream->mpuExtendedTimestamps[i].numOfAu = ts.numOfAu;
                mmtpStream->mpuExtendedTimestamps[i].ptsOffsets = ts.ptsOffsets;
                mmtpStream->mpuExtendedTimestamps[i].dtsPtsOffsets = ts.dtsPtsOffsets;
                find = true;
                break;
            }
        }
        if (find) {
            continue;
        }

        if (mmtpStream->mpuExtendedTimestamps.size() >= 100) {
            int minIndex = 0;
            uint32_t minMpuSequenceNumber = 0xFFFFFFFF;
            for (int i = 0; i < mmtpStream->mpuExtendedTimestamps.size(); i++) {
                if (minMpuSequenceNumber > mmtpStream->mpuExtendedTimestamps[i].mpuSequenceNumber) {
                    minIndex = i;
                    minMpuSequenceNumber = mmtpStream->mpuExtendedTimestamps[i].mpuSequenceNumber;
                }
            }

            mmtpStream->mpuExtendedTimestamps[minIndex].mpuSequenceNumber = ts.mpuSequenceNumber;
            mmtpStream->mpuExtendedTimestamps[minIndex].mpuDecodingTimeOffset = ts.mpuDecodingTimeOffset;
            mmtpStream->mpuExtendedTimestamps[minIndex].numOfAu = ts.numOfAu;
            mmtpStream->mpuExtendedTimestamps[minIndex].ptsOffsets = ts.ptsOffsets;
            mmtpStream->mpuExtendedTimestamps[minIndex].dtsPtsOffsets = ts.dtsPtsOffsets;
        }
        else {
            mmtpStream->mpuExtendedTimestamps.push_back(ts);
        }
    }
}

void MmtTlvDemuxer::processEcm(Ecm* ecm)
{
    try {
        acasCard->decryptEcm(ecm->ecmData);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
}

void MmtTlvDemuxer::clearTables()
{
    for (auto table : tables) {
        delete table;
    }
    tables.clear();

    for (auto table : tlvTables) {
        delete table;
    }
    tlvTables.clear();
}

void MmtTlvDemuxer::clear()
{
    clearTables();

    assemblerMap.clear();
    mpuData.clear();
    streamMap.clear();
    streamIndex = 0;

    if (acasCard) {
        acasCard->clear();
    }
}

std::pair<int64_t, int64_t> MmtTlvDemuxer::calcPtsDts(MmtpStream* mmtpStream, MpuTimestampDescriptor::Entry& timestamp, MpuExtendedTimestampDescriptor::Entry& extendedTimestamp)
{
    int64_t ptime = av_rescale(timestamp.mpuPresentationTime, mmtpStream->timeBase.den,
        1000000ll * mmtpStream->timeBase.num);

    if (mmtpStream->auIndex >= extendedTimestamp.numOfAu) {
        throw std::out_of_range("au index out of bounds");
    }

    int64_t dts = ptime - extendedTimestamp.mpuDecodingTimeOffset;
    for (int j = 0; j < mmtpStream->auIndex; ++j)
        dts += extendedTimestamp.ptsOffsets[j];

    int64_t pts = dts + extendedTimestamp.dtsPtsOffsets[mmtpStream->auIndex];

    return std::pair<int64_t, int64_t>(pts, dts);
}

FragmentAssembler* MmtTlvDemuxer::getAssembler(uint16_t pid)
{
    if (assemblerMap.find(pid) == assemblerMap.end()) {
        FragmentAssembler* assembler = new FragmentAssembler();
        assemblerMap[pid] = assembler;
        return assembler;
    }
    else {
        return assemblerMap[pid];
    }
}

MmtpStream* MmtTlvDemuxer::getStream(uint16_t pid, bool create)
{
    if (streamMap.find(pid) == streamMap.end()) {
        if (create) {
            MmtpStream* stream = new MmtpStream();
            stream->streamIndex = streamIndex++;
            stream->pid = pid;
            streamMap[pid] = stream;
            return stream;
        }
        else {
            return nullptr;
        }
    }
    else {
        return streamMap[pid];
    }

}

void MmtTlvDemuxer::processMpu(Stream& stream)
{
    if (!mpu.unpack(stream))
        return;

    auto assembler = getAssembler(mmtp.packetId);
    Stream nstream(mpu.payload);
    MmtpStream* mmtpStream = getStream(mmtp.packetId);

    if (!mmtpStream) {
        return;
    }

    if (mpu.aggregateFlag && mpu.fragmentationIndicator != NOT_FRAGMENTED) {
        return;
    }

    if (mpu.fragmentType != MPU_FRAGMENT_TYPE::MPU) {
        return;
    }

    if (assembler->state == ASSEMBLER_STATE::INIT && !mmtp.rapFlag) {
        return;
    }

    if (assembler->state == ASSEMBLER_STATE::INIT) {
        mmtpStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
    }
    else if (mpu.mpuSequenceNumber == mmtpStream->lastMpuSequenceNumber + 1) {
        mmtpStream->lastMpuSequenceNumber = mpu.mpuSequenceNumber;
        mmtpStream->auIndex = 0;
    }
    else if (mpu.mpuSequenceNumber != mmtpStream->lastMpuSequenceNumber) {
        std::cerr << "drop found (" << mmtpStream->lastMpuSequenceNumber << " != " << mpu.mpuSequenceNumber << ")" << std::endl;
        assembler->state = ASSEMBLER_STATE::INIT;
        return;
    }

    assembler->checkState(mmtp.packetSequenceNumber);

    if (mmtp.rapFlag) {
        mmtpStream->flags |= AV_PKT_FLAG_KEY;
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
        int i = 0;
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
            i++;
        }
    }
}

static void buffer_free_callback(void* opaque, uint8_t* data) {
    free(data);
}

void MmtTlvDemuxer::processMpuData(Stream& stream)
{
    MmtpStream* mmtpStream = getStream(mmtp.packetId);
    if (!mmtpStream) {
        return;
    }

    MpuTimestampDescriptor::Entry timestamp;
    MpuExtendedTimestampDescriptor::Entry extendedTimestamp;

    if (mmtpStream->codecId == AV_CODEC_ID_HEVC || mmtpStream->codecId == AV_CODEC_ID_AAC_LATM) {
        bool findTimestamp = false, findExtendedTimestamp = false;
        for (int i = 0; i < mmtpStream->mpuTimestamps.size(); ++i) {
            if (mmtpStream->mpuTimestamps[i].mpuSequenceNumber ==
                mmtpStream->lastMpuSequenceNumber) {
                timestamp = mmtpStream->mpuTimestamps[i];
                findTimestamp = true;
                break;
            }
        }

        for (int i = 0; i < mmtpStream->mpuExtendedTimestamps.size(); ++i) {
            if (mmtpStream->mpuExtendedTimestamps[i].mpuSequenceNumber ==
                mmtpStream->lastMpuSequenceNumber) {
                extendedTimestamp = mmtpStream->mpuExtendedTimestamps[i];
                findExtendedTimestamp = true;
                break;
            }
        }
        if (!findTimestamp || !findExtendedTimestamp) {
            return;
        }
    }

    switch (mmtpStream->codecId) {
    case AV_CODEC_ID_HEVC:
    {
        if (stream.leftBytes() < 4) {
            return;
        }

        uint32_t size = stream.getBe32U();
        if (size != stream.leftBytes()) {
            return;
        }

        uint8_t uint8 = stream.peek8U();
        if ((uint8 >> 7) != 0) {
            return;
        }

        int oldSize = mmtpStream->pendingData.size();
        if (oldSize != 0)
            oldSize -= 64;

        uint32_t nalHeader = 0x1000000;
        mmtpStream->pendingData.resize(oldSize + 4 + size + 64);
        memcpy(mmtpStream->pendingData.data() + oldSize, &nalHeader, 4);
        stream.read((char*)mmtpStream->pendingData.data() + oldSize + 4, size);

        memset(mmtpStream->pendingData.data() + oldSize + 4 + size, 0, 64);

        if (((uint8 >> 1) & 0b111111) < 0x20) {
            uint8_t* data = (uint8_t*)malloc(mmtpStream->pendingData.size());
            if (data == 0) {
                return;
            }

            memcpy(data, mmtpStream->pendingData.data(), mmtpStream->pendingData.size());

            AVBufferRef* buf = av_buffer_create(data, mmtpStream->pendingData.size(), buffer_free_callback, NULL, 0);
            AVPacket* packet = av_packet_alloc();
            packet->buf = buf;
            packet->data = buf->data;
            packet->size = mmtpStream->pendingData.size() - 64;

            std::pair<int64_t, int64_t> ptsDts;
            try {
                ptsDts = calcPtsDts(mmtpStream, timestamp, extendedTimestamp);
            }
            catch (const std::out_of_range& e) {
                std::cerr << e.what() << std::endl;
                return;
            }

            mmtpStream->auIndex++;
            packet->pts = ptsDts.first;
            packet->dts = ptsDts.second;
            packet->stream_index = mmtpStream->streamIndex;
            packet->flags = mmtpStream->flags;
            packet->pos = -1;
            packet->duration = 0;
            packet->size = buf->size - AV_INPUT_BUFFER_PADDING_SIZE;
            avpackets.push_back(packet);

            mmtpStream->pendingData.clear();
            mmtpStream->flags = 0;
        }

        break;
    }
    case AV_CODEC_ID_AAC_LATM:
    {
        uint32_t size = stream.leftBytes();

        uint8_t* data = (uint8_t*)malloc(3 + size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (data == 0) {
            return;
        }

        data[0] = 0x56;
        data[1] = ((size >> 8) & 0x1F) | 0xE0;
        data[2] = size & 0xFF;

        stream.read((char*)data + 3, size);

        memset(data + 3 + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

        AVBufferRef* buf = av_buffer_create(data, 3 + size + AV_INPUT_BUFFER_PADDING_SIZE, buffer_free_callback, NULL, 0);
        AVPacket* packet = av_packet_alloc();
        packet->buf = buf;
        packet->data = buf->data;


        std::pair<int64_t, int64_t> ptsDts;
        try {
            ptsDts = calcPtsDts(mmtpStream, timestamp, extendedTimestamp);
        }
        catch (const std::out_of_range&) {
            return;
        }

        mmtpStream->auIndex++;
        packet->pts = ptsDts.first;
        packet->dts = ptsDts.second;
        packet->stream_index = mmtpStream->streamIndex;
        packet->flags = mmtpStream->flags;
        packet->pos = -1;
        packet->duration = 0;
        packet->size = buf->size - AV_INPUT_BUFFER_PADDING_SIZE;
        avpackets.push_back(packet);
        mmtpStream->flags = 0;
        break;
    }
    case AV_CODEC_ID_TTML:
    {
        uint32_t size = stream.leftBytes();

        uint16_t subsampleNumber = stream.getBe16U();
        uint16_t lastSubsampleNumber = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        uint8_t dataType = uint8 >> 4;
        uint8_t lengthExtFlag = (uint8 >> 3) & 1;
        uint8_t subsampleInfoListFlag = (uint8 >> 2) & 1;

        if (dataType != 0) {
            return;
        }

        uint32_t data_size;
        if (lengthExtFlag)
            data_size = stream.getBe32U();
        else
            data_size = stream.getBe16U();

        if (subsampleNumber == 0 && lastSubsampleNumber > 0 && subsampleInfoListFlag) {
            for (int i = 0; i < lastSubsampleNumber; ++i) {
                // skip: subsample_i_data_type
                stream.skip(4 + 4);

                // skip: subsample_i_data_size
                if (lengthExtFlag) {
                    stream.skip(4);
                }
                else {
                    stream.skip(2);
                }
            }
        }

        if (stream.leftBytes() < data_size) {
            return;
        }

        uint8_t* data = (uint8_t*)malloc(data_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (data == 0) {
            return;
        }

        stream.read((char*)data, data_size);
        memset(data + data_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

        AVBufferRef* buf = av_buffer_create(data, data_size + AV_INPUT_BUFFER_PADDING_SIZE, buffer_free_callback, NULL, 0);

        AVPacket* packet = av_packet_alloc();
        packet->stream_index = mmtpStream->streamIndex;
        packet->buf = buf;
        packet->data = buf->data;
        packet->size = data_size - AV_INPUT_BUFFER_PADDING_SIZE;
        packet->flags = mmtpStream->flags;
        packet->pos = -1;
        avpackets.push_back(packet);

        mmtpStream->flags = 0;
        break;
    }
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

bool MmtTlvDemuxer::isVaildTlv(Stream& stream) const
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
    catch (const std::runtime_error& e) {
        return false;
    }
    return true;
}

bool FragmentAssembler::assemble(std::vector<uint8_t> fragment, uint8_t fragmentationIndicator, uint32_t packetSequenceNumber)
{
    switch (fragmentationIndicator) {
    case NOT_FRAGMENTED:
        if (state == ASSEMBLER_STATE::IN_FRAGMENT)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        state = ASSEMBLER_STATE::NOT_STARTED;
        return true;
    case FIRST_FRAGMENT:
        if (state == ASSEMBLER_STATE::IN_FRAGMENT) {
            return false;
        }

        state = ASSEMBLER_STATE::IN_FRAGMENT;
        data.insert(data.end(), fragment.begin(), fragment.end());
        break;
    case MIDDLE_FRAGMENT:
        if (state == ASSEMBLER_STATE::SKIP) {
            std::cerr << "Drop packet " << packetSequenceNumber << std::endl;
            return false;
        }

        if (state != ASSEMBLER_STATE::IN_FRAGMENT)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        break;
    case LAST_FRAGMENT:
        if (state == ASSEMBLER_STATE::SKIP) {
            std::cerr << "Drop packet " << packetSequenceNumber << std::endl;
            return false;
        }

        if (state != ASSEMBLER_STATE::IN_FRAGMENT)
            return false;

        data.insert(data.end(), fragment.begin(), fragment.end());
        state = ASSEMBLER_STATE::NOT_STARTED;
        return true;
    }

    return false;
}

void FragmentAssembler::checkState(uint32_t packetSequenceNumber)
{
    if (state == ASSEMBLER_STATE::INIT) {
        state = ASSEMBLER_STATE::SKIP;
    }
    else if (packetSequenceNumber != last_seq + 1) {
        if (data.size() != 0) {
            std::cerr << "Packet sequence number jump: " << last_seq << " + 1 != " << packetSequenceNumber << ", drop " << data.size() << " bytes\n" << std::endl;
            data.clear();
        }
        else {
            std::cerr <<
                "Packet sequence number jump: " << last_seq << " + 1 != " << packetSequenceNumber << std::endl;
        }

        state = ASSEMBLER_STATE::SKIP;
    }
    last_seq = packetSequenceNumber;
}

void FragmentAssembler::clear()
{
    state = ASSEMBLER_STATE::NOT_STARTED;
    data.clear();
}
