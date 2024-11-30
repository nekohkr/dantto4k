#include "remuxerHandler.h"
#include "accessControlDescriptor.h"
#include "contentCopyControlDescriptor.h"
#include "mhAudioComponentDescriptor.h"
#include "mhContentDescriptor.h"
#include "mhEit.h"
#include "mhEventGroupDescriptor.h"
#include "mhExtendedEventDescriptor.h"
#include "mhLinkageDescriptor.h"
#include "mhLogoTransmissionDescriptor.h"
#include "mhParentalRatingDescriptor.h"
#include "mhSeriesDescriptor.h"
#include "mhShortEventDescriptor.h"
#include "mhSiParameterDescriptor.h"
#include "mhStreamIdentificationDescriptor.h"
#include "mmtTableBase.h"
#include "multimediaServiceInformationDescriptor.h"
#include "networkNameDescriptor.h"
#include "relatedBroadcasterDescriptor.h"
#include "serviceListDescriptor.h"
#include "videoComponentDescriptor.h"
#include "mhServiceDescriptor.h"
#include "descriptorConverter.h"
#include "mhBit.h"
#include "mhCdt.h"
#include "mhSdt.h"
#include "mhTot.h"
#include "mpt.h"
#include "nit.h"
#include "plt.h"
#include "pesPacket.h"
#include "adtsConverter.h"
#include "mmtTlvDemuxer.h"
#include "timebase.h"
#include "mfuDataProcessorBase.h"
#include "config.h"
#include "ntp.h"
#include "logger.h"

namespace {
int convertRunningStatus(int runningStatus) {
    /*
       MMT
       0 undefined
       1 In non-operation
       2 It will start within several seconds
        (ex: video recording use)
       3 Out of operation
       4 In operation
       5 – 7 Reserved for use in the future

       MPEG TS
       GST_MPEGTS_RUNNING_STATUS_UNDEFINED (0)
       GST_MPEGTS_RUNNING_STATUS_NOT_RUNNING (1)
       GST_MPEGTS_RUNNING_STATUS_STARTS_IN_FEW_SECONDS (2)
       GST_MPEGTS_RUNNING_STATUS_PAUSING (3)
       GST_MPEGTS_RUNNING_STATUS_RUNNING (4)
       GST_MPEGTS_RUNNING_STATUS_OFF_AIR (5)
   */

    switch (runningStatus) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 5;
    case 4:
        return 4;
    default:
        return 0;
    }
}

int assetType2streamType(uint32_t assetType)
{
    int stream_type = 0;
    switch (assetType) {
    case MmtTlv::AssetType::hev1:
        stream_type = STREAM_TYPE_VIDEO_HEVC;
        break;
    case MmtTlv::AssetType::mp4a:
        stream_type = config.disableADTSConversion ? STREAM_TYPE_AUDIO_AAC_LATM : STREAM_TYPE_AUDIO_AAC;
        break;
    case MmtTlv::AssetType::stpp:
        stream_type = STREAM_TYPE_PRIVATE_DATA;
        break;
    }

    return stream_type;
}

uint8_t convertTableId(uint8_t mmtTableId) {
    switch (mmtTableId) {
    case MmtTlv::MmtTableId::Mpt:
        return 0x02;
    case MmtTlv::MmtTableId::Plt:
        return 0x00;
    case MmtTlv::MmtTableId::MhEitPf:
        return 0x4E;
    case MmtTlv::MmtTableId::MhEitS_1:
    case MmtTlv::MmtTableId::MhEitS_2:
    case MmtTlv::MmtTableId::MhEitS_3:
    case MmtTlv::MmtTableId::MhEitS_4:
    case MmtTlv::MmtTableId::MhEitS_5:
    case MmtTlv::MmtTableId::MhEitS_6:
    case MmtTlv::MmtTableId::MhEitS_7:
    case MmtTlv::MmtTableId::MhEitS_8:
    case MmtTlv::MmtTableId::MhEitS_9:
    case MmtTlv::MmtTableId::MhEitS_10:
    case MmtTlv::MmtTableId::MhEitS_11:
    case MmtTlv::MmtTableId::MhEitS_12:
    case MmtTlv::MmtTableId::MhEitS_13:
    case MmtTlv::MmtTableId::MhEitS_14:
    case MmtTlv::MmtTableId::MhEitS_15:
    case MmtTlv::MmtTableId::MhEitS_16:
        return 0x50;
    case MmtTlv::MmtTableId::MhTot:
        return 0x73;
    case MmtTlv::MmtTableId::MhBit:
        return 0xC4;
    case MmtTlv::MmtTableId::MhSdt:
        return 0x42;
    case MmtTlv::MmtTableId::MhCdt:
        return 0xC8;
    }

    return 0xFF;
}

} // anonymous namespace

void RemuxerHandler::onVideoData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData)
{
    writeStream(mmtStream, mfuData, mfuData->data);
}

void RemuxerHandler::onAudioData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData)
{
    if (config.disableADTSConversion) {
        writeStream(mmtStream, mfuData, mfuData->data);
        return;
    }

    ADTSConverter converter;
    std::vector<uint8_t> output;
    if (!converter.convert(mfuData->data.data(), mfuData->data.size(), output)) {
        return;
    }

    writeStream(mmtStream, mfuData, output);
}

void RemuxerHandler::onSubtitleData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData)
{
    writeStream(mmtStream, mfuData, mfuData->data);
}

void RemuxerHandler::onApplicationData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData)
{
}

void RemuxerHandler::writeStream(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData, const std::vector<uint8_t>& streamData)
{
    const AVRational tsTimeBase = { 1, 90000 };
    AVRational timeBase = { mmtStream->timeBase.num, mmtStream->timeBase.den };

    uint64_t tsPts = 0;
    uint64_t tsDts = 0;

    if(mfuData->pts && mfuData->dts && timeBase.den > 0) {
        tsPts = av_rescale_q(mfuData->pts, timeBase, tsTimeBase);
        tsDts = av_rescale_q(mfuData->dts, timeBase, tsTimeBase);
    }

    std::vector<uint8_t> pesOutput;
    PESPacket pes;
    pes.setPts(tsPts);
    pes.setDts(tsDts);
    pes.setStreamId(componentTagToStreamId(mmtStream->getComponentTag()));
    if (mmtStream->getAssetType() == MmtTlv::AssetType::hev1 ||
        mmtStream->getAssetType() == MmtTlv::AssetType::mp4a) {
        pes.setDataAlignmentIndicator(true);
    }
    pes.setPayload(&streamData);
    pes.pack(pesOutput);

    ts::PESPacket tsPES(pesOutput.data(), pesOutput.size());

    ts::PESOneShotPacketizer zer(duck, mmtStream->getMpeg2Pid());
    zer.addPES(tsPES, ts::ShareMode::SHARE);

    ts::TSPacketVector packets;
    zer.getPackets(packets);

    for (auto& packet : packets) {
        packet.setCC(mapCC[mmtStream->getMpeg2Pid()] & 0xF);
        ++mapCC[mmtStream->getMpeg2Pid()];

        output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
    }
}

void RemuxerHandler::onMhBit(const std::shared_ptr<MmtTlv::MhBit>& mhBit)
{
    ts::BIT tsBit(mhBit->versionNumber, mhBit->currentNextIndicator);
    tsBit.original_network_id = mhBit->originalNetworkId;
    
    for (const auto& descriptor : mhBit->descriptors.list) {
        switch (descriptor->getDescriptorTag()) {
        case MmtTlv::MhSiParameterDescriptor::kDescriptorTag:
        {
            auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhSiParameterDescriptor>(descriptor);
            ts::SIParameterDescriptor tsDescriptor;
            tsDescriptor.parameter_version = mmtDescriptor->parameterVersion;
            
            struct tm tm;
            EITDecodeMjd(mmtDescriptor->updateTime, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);

            try {
                tsDescriptor.update_time = ts::Time(tm.tm_year, tm.tm_mon, tm.tm_mday, 0, 0);
            }
            catch (ts::Time::TimeError) {
                return;
            }

            for (const auto& entry : mmtDescriptor->entries) {
                ts::SIParameterDescriptor::Entry tsEntry;
                tsEntry.table_id = convertTableId(entry.tableId);
                if (tsEntry.table_id == 0xFF) {
                    continue;
                }

                tsEntry.table_description.resize(entry.tableDescriptionByte.size());
                memcpy(tsEntry.table_description.data(), entry.tableDescriptionByte.data(), entry.tableDescriptionByte.size());
                tsDescriptor.entries.push_back(tsEntry);
            }
            
            tsBit.descs.add(duck, tsDescriptor);
            break;
        }
        }
    }

    for (const auto& broadcaster : mhBit->broadcasters) {
        auto& tsBroadcaster = tsBit.broadcasters[broadcaster.broadcasterId];

        for (const auto& descriptor : broadcaster.descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MmtTlv::RelatedBroadcasterDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::RelatedBroadcasterDescriptor>(descriptor);
                ts::ExtendedBroadcasterDescriptor tsDescriptor;
                tsDescriptor.broadcaster_type = 1;
                tsDescriptor.terrestrial_broadcaster_id = mhBit->originalNetworkId;

                for (auto affiliationId : mmtDescriptor->affiliationIds) {
                    tsDescriptor.affiliation_ids.emplace_back(affiliationId);
                }
                
                for (const auto& broadcasterId : mmtDescriptor->broadcasterIds) {
                    tsDescriptor.broadcasters.emplace_back(broadcasterId.networkId, broadcasterId.broadcasterId);
                }
            
                tsBit.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhSiParameterDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhSiParameterDescriptor>(descriptor);
                ts::SIParameterDescriptor tsDescriptor;
                tsDescriptor.parameter_version = mmtDescriptor->parameterVersion;
            
                struct tm tm;
                EITDecodeMjd(mmtDescriptor->updateTime, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);

                try {
                    tsDescriptor.update_time = ts::Time(tm.tm_year, tm.tm_mon, tm.tm_mday, 0, 0);
                }
                catch (ts::Time::TimeError) {
                    return;
                }

                for (const auto& entry : mmtDescriptor->entries) {
                    ts::SIParameterDescriptor::Entry tsEntry;
                    tsEntry.table_id = convertTableId(entry.tableId);
                    if (tsEntry.table_id == 0xFF) {
                        continue;
                    }

                    tsEntry.table_description.resize(entry.tableDescriptionByte.size());
                    memcpy(tsEntry.table_description.data(), entry.tableDescriptionByte.data(), entry.tableDescriptionByte.size());
                    tsDescriptor.entries.push_back(tsEntry);
                }
            
                tsBit.descs.add(duck, tsDescriptor);
                break;
            }
            }
        }

    }

    ts::BinaryTable table;
    tsBit.serialize(duck, table);

    ts::OneShotPacketizer packetizer(duck, ISDB_BIT_PID);
    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);
        section->setSectionNumber(mhBit->sectionNumber);
        section->setLastSectionNumber(mhBit->lastSectionNumber);

        packetizer.addSection(section);
        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[ISDB_BIT_PID] & 0xF);
            mapCC[ISDB_BIT_PID]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onMhEit(const std::shared_ptr<MmtTlv::MhEit>& mhEit)
{
    tsid = mhEit->tlvStreamId;

    ts::EIT tsEit(true, mhEit->isPf(), 0, mhEit->versionNumber, true, mhEit->serviceId, mhEit->tlvStreamId, mhEit->originalNetworkId);
	for (auto& mhEvent : mhEit->events) {
		ts::EIT::Event tsEvent(&tsEit);

        struct tm startTime = EITConvertStartTime(mhEvent->startTime);
        try {
            tsEvent.start_time = ts::Time(startTime.tm_year + 1900, startTime.tm_mon + 1, startTime.tm_mday,
                startTime.tm_hour, startTime.tm_min, startTime.tm_sec);
        }
        catch(ts::Time::TimeError){
            return;
        }

        tsEvent.duration = std::chrono::seconds(EITConvertDuration(mhEvent->duration));
        tsEvent.running_status = convertRunningStatus(mhEvent->runningStatus);
        tsEvent.event_id = mhEvent->eventId;

        for (const auto& descriptor : mhEvent->descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MmtTlv::MhShortEventDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhShortEventDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhShortEventDescriptor>::convert(*mmtDescriptor);
                if (tsDescriptor) {
                    tsEvent.descs.add(tsDescriptor->data(), tsDescriptor->size());
                }
                break;
            }
            case MmtTlv::MhExtendedEventDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhExtendedEventDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhExtendedEventDescriptor>::convert(*mmtDescriptor);
                if(tsDescriptor) {
                    tsEvent.descs.add(tsDescriptor->data(), tsDescriptor->size());
                }
                break;
            }
            case MmtTlv::MhAudioComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhAudioComponentDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhAudioComponentDescriptor>::convert(*mmtDescriptor);
                
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::VideoComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::VideoComponentDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::VideoComponentDescriptor>::convert(*mmtDescriptor);
                
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhContentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhContentDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhContentDescriptor>::convert(*mmtDescriptor);
               
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhLinkageDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhLinkageDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhLinkageDescriptor>::convert(*mmtDescriptor);

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhEventGroupDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhEventGroupDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhEventGroupDescriptor>::convert(*mmtDescriptor);

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhParentalRatingDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhParentalRatingDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhParentalRatingDescriptor>::convert(*mmtDescriptor);

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhSeriesDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhSeriesDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhSeriesDescriptor>::convert(*mmtDescriptor);
                
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::ContentCopyControlDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::ContentCopyControlDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::ContentCopyControlDescriptor>::convert(*mmtDescriptor);

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MultimediaServiceInformationDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MultimediaServiceInformationDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MultimediaServiceInformationDescriptor>::convert(*mmtDescriptor);
                
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            }
        }

        tsEit.events[tsEvent.event_id] = tsEvent;
	}

    ts::BinaryTable table;
    tsEit.serialize(duck, table);

    ts::OneShotPacketizer packetizer(duck, DVB_EIT_PID);
    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);
        section.get()->setSectionNumber(mhEit->sectionNumber);
        section.get()->setLastSectionNumber(mhEit->lastSectionNumber);

        packetizer.addSection(section);
        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[DVB_EIT_PID] & 0xF);
            mapCC[DVB_EIT_PID]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onMhSdt(const std::shared_ptr<MmtTlv::MhSdt>& mhSdt)
{
    if (mhSdt->services.size() == 0) {
        return;
    }

    tsid = mhSdt->tlvStreamId;

    ts::SDT tsSdt(true, mhSdt->versionNumber, mhSdt->currentNextIndicator, mhSdt->tlvStreamId, mhSdt->originalNetworkId);
    for (auto& service : mhSdt->services) {
        ts::SDT::ServiceEntry tsService(&tsSdt);
        tsService.EITs_present = service->eitScheduleFlag;
        tsService.EITpf_present = service->eitPresentFollowingFlag;
        tsService.running_status = convertRunningStatus(service->runningStatus);
        tsService.CA_controlled = service->freeCaMode;

        for (auto& descriptor : service->descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MmtTlv::MhServiceDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhServiceDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhServiceDescriptor>::convert(*mmtDescriptor);
               
                tsService.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhLogoTransmissionDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhLogoTransmissionDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::MhLogoTransmissionDescriptor>::convert(*mmtDescriptor);
                
                tsService.descs.add(duck, tsDescriptor);
                break;
            }
            }
        }
        tsSdt.services[service->serviceId] = tsService;
    }

    ts::BinaryTable table;
    tsSdt.serialize(duck, table);

    ts::OneShotPacketizer packetizer(duck, DVB_SDT_PID);
    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);
        section.get()->setSectionNumber(mhSdt->sectionNumber);
        section.get()->setLastSectionNumber(mhSdt->lastSectionNumber);

        packetizer.addSection(section);
        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[DVB_SDT_PID] & 0xF);
            mapCC[DVB_SDT_PID]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onPlt(const std::shared_ptr<MmtTlv::Plt>& plt)
{
    if (tsid == -1)
        return;

    ts::PAT pat(plt->version % 32, true, tsid);
    mapService2Pid.clear();

    int i = 0;
    for (auto& item : plt->entries) {
        if (item.mmtPackageIdLength != 2) {
            return;
        }

        uint16_t serviceId = MmtTlv::Common::swapEndian16(*(uint16_t*)item.mmtPackageIdByte.data());

        if (item.locationInfos.locationType != 0) {
            return;
        }

        pat.pmts[serviceId] = 0x1000 + i;
        mapService2Pid[serviceId] = 0x1000 + i;
        i++;
    }

    ts::BinaryTable table;
    pat.serialize(duck, table);

    ts::OneShotPacketizer packetizer(duck, MPEG_PAT_PID);

    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);
        packetizer.addSection(section);

        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[MPEG_PAT_PID] & 0xF);
            mapCC[MPEG_PAT_PID]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onMpt(const std::shared_ptr<MmtTlv::Mpt>& mpt)
{
    uint16_t serviceId;
    uint16_t pid;

    if (mpt->mmtPackageIdLength != 2) {
        return;
    }

    serviceId = MmtTlv::Common::swapEndian16(*(uint16_t*)mpt->mmtPackageIdByte.data());

    auto it = mapService2Pid.find(serviceId);
    if (it == mapService2Pid.end()) {
        return;
    }

    pid = it->second;

    ts::PMT tsPmt(mpt->version % 32, true, serviceId, PCR_PID);

    // For VLC to recognize as ARIB standard
    ts::CADescriptor caDescriptor(5, 0x0901);
    tsPmt.descs.add(duck, caDescriptor);

    int streamIndex = 0;
    for (auto& asset : mpt->assets) {
        for (int i = 0; i < asset.locationCount; i++) {
            if (asset.locationInfos[i].locationType == 0) {
                int streamType = assetType2streamType(asset.assetType);
                if (streamType == 0) {
                    continue;
                }

                ts::PMT::Stream stream(&tsPmt, streamType);
                
                if (streamType == STREAM_TYPE_VIDEO_HEVC) {
                    ts::RegistrationDescriptor descriptor;
                    descriptor.format_identifier = 0x48455643; // HEVC
                    stream.descs.add(duck, descriptor);
                }

                for (const auto& descriptor : asset.descriptors.list) {
                    switch (descriptor->getDescriptorTag()) {
                    case MmtTlv::MhStreamIdentificationDescriptor::kDescriptorTag:
                    {
                        auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhStreamIdentificationDescriptor>(descriptor);
                        auto tsDescriptor = DescriptorConverter<MmtTlv::MhStreamIdentificationDescriptor>::convert(*mmtDescriptor);
                        
                        stream.descs.add(duck, tsDescriptor);
                        break;
                    }
                    }
                }

                const auto& mmtStream = demuxer.mapStreamByStreamIdx[streamIndex];
                if (mmtStream->getComponentTag() == -1) {
                    streamIndex++;
                    continue;
                }

                tsPmt.streams[mmtStream->getMpeg2Pid()] = stream;
                streamIndex++;
            }
        }
    }

    for (const auto& descriptor : mpt->descriptors.list) {
        switch (descriptor->getDescriptorTag()) {
        case MmtTlv::AccessControlDescriptor::kDescriptorTag:
        {
            auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::AccessControlDescriptor>(descriptor);
            auto tsDescriptor = DescriptorConverter<MmtTlv::AccessControlDescriptor>::convert(*mmtDescriptor);
            
            tsPmt.descs.add(duck, tsDescriptor);
            break;
        }
        
        case MmtTlv::ContentCopyControlDescriptor::kDescriptorTag:
        {
            auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::ContentCopyControlDescriptor>(descriptor);
            auto tsDescriptor = DescriptorConverter<MmtTlv::ContentCopyControlDescriptor>::convert(*mmtDescriptor);
            
            tsPmt.descs.add(duck, tsDescriptor);
            break;
        }
        }
    }

    ts::BinaryTable table;
    tsPmt.serialize(duck, table);

    ts::OneShotPacketizer packetizer(duck, pid);

    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);
        packetizer.addSection(section);

        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[pid] & 0xF);
            mapCC[pid]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onMhTot(const std::shared_ptr<MmtTlv::MhTot>& mhTot)
{
    struct tm startTime = EITConvertStartTime(mhTot->jstTime);
    ts::Time time = ts::Time(startTime.tm_year + 1900, startTime.tm_mon + 1, startTime.tm_mday,
        startTime.tm_hour, startTime.tm_min, startTime.tm_sec);
    ts::TOT tot(time);

    ts::BinaryTable table;
    tot.serialize(duck, table);
    ts::OneShotPacketizer packetizer(duck, DVB_TOT_PID);

    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);

        packetizer.addSection(section);

        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[DVB_TOT_PID] & 0xF);
            mapCC[DVB_TOT_PID]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onMhCdt(const std::shared_ptr<MmtTlv::MhCdt>& mhCdt)
{
    ts::CDT cdt(mhCdt->versionNumber, mhCdt->currentNextIndicator);
    cdt.original_network_id = mhCdt->originalNetworkId;
    cdt.download_data_id = mhCdt->downloadDataId;
    cdt.data_type = mhCdt->dataType;
    cdt.data_module.resize(mhCdt->dataModuleByte.size());
    memcpy(cdt.data_module.data(), mhCdt->dataModuleByte.data(), mhCdt->dataModuleByte.size());

    ts::BinaryTable table;
    cdt.serialize(duck, table);
    ts::OneShotPacketizer packetizer(duck, ISDB_CDT_PID);

    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);
        section.get()->setSectionNumber(mhCdt->sectionNumber);
        section.get()->setLastSectionNumber(mhCdt->lastSectionNumber);
        packetizer.addSection(section);

        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[ISDB_CDT_PID] & 0xF);
            mapCC[ISDB_CDT_PID]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onNit(const std::shared_ptr<MmtTlv::Nit>& nit)
{
    ts::NIT tsNit(true, nit->versionNumber, nit->currentNextIndicator, nit->networkId);

    for (const auto& descriptor : nit->descriptors.list) {
        switch (descriptor->getDescriptorTag()) {
        case MmtTlv::NetworkNameDescriptor::kDescriptorTag:
        {
            auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::NetworkNameDescriptor>(descriptor);
            auto tsDescriptor = DescriptorConverter<MmtTlv::NetworkNameDescriptor>::convert(*mmtDescriptor);
            
            tsNit.descs.add(duck, tsDescriptor);
            break;
        }
        }
    }

    for (const auto& item : nit->entries) {
        ts::TransportStreamId tsid(item.tlvStreamId, item.originalNetworkId);
        tsNit.transports[tsid];

        for (auto& descriptor : item.descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MmtTlv::ServiceListDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::ServiceListDescriptor>(descriptor);
                auto tsDescriptor = DescriptorConverter<MmtTlv::ServiceListDescriptor>::convert(*mmtDescriptor);
                
                tsNit.transports[tsid].descs.add(duck, tsDescriptor);
                break;
            }
            }
        }
    }

    ts::BinaryTable table;
    tsNit.serialize(duck, table);
    ts::OneShotPacketizer packetizer(duck, DVB_NIT_PID);

    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);
        section->setSectionNumber(nit->sectionNumber);
        section->setLastSectionNumber(nit->lastSectionNumber);
        packetizer.addSection(section);

        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[DVB_NIT_PID] & 0xF);
            mapCC[DVB_NIT_PID]++;

            output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());
        }
    }
}

void RemuxerHandler::onNtp(const std::shared_ptr<MmtTlv::NTPv4>& ntp)
{
    //if(nextPcrTs > ntp->transmit_timestamp.toPCRValue()) {
    //    return;
    //}

    ts::TSPacket packet;
    packet.init(PCR_PID, mapCC[PCR_PID] & 0xF, 0);
    mapCC[PCR_PID]++;
    packet.setPCR(ntp->transmit_timestamp.toPCRValue() , true);
    output.insert(output.end(), packet.b, packet.b + packet.getHeaderSize() + packet.getPayloadSize());

    //nextPcrTs = ntp->transmit_timestamp.toPCRValue() + 27000 * 100;
}

void RemuxerHandler::clear()
{
    mapService2Pid.clear();
    mapCC.clear();
    tsid = -1;
    streamCount = 0;
    nextPcrTs = 0;
}
