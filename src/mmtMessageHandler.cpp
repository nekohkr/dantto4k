#include "mmtMessageHandler.h"
#include "mhEit.h"
#include "mhExtendedEventDescriptor.h"
#include "mhShortEventDescriptor.h"
#include "mhContentDescriptor.h"
#include "mhAudioComponentDescriptor.h"
#include "videoComponentDescriptor.h"
#include "mhSdt.h"
#include "mhServiceDescriptor.h"
#include "plt.h"
#include "tlvNit.h"
#include "mpt.h"
#include "mhTot.h"

#include "mmttlvdemuxer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

static int convertRunningStatus(int runningStatus) {
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

static void EITDecodeMjd(int i_mjd, int* p_y, int* p_m, int* p_d)
{
    const int yp = (int)(((double)i_mjd - 15078.2) / 365.25);
    const int mp = (int)(((double)i_mjd - 14956.1 - (int)(yp * 365.25)) / 30.6001);
    const int c = (mp == 14 || mp == 15) ? 1 : 0;

    *p_y = 1900 + yp + c * 1;
    *p_m = mp - 1 - c * 12;
    *p_d = i_mjd - 14956 - (int)(yp * 365.25) - (int)(mp * 30.6001);
}

#define CVT_FROM_BCD(v) ((((v) >> 4)&0xf)*10 + ((v)&0xf))
static struct tm EITConvertStartTime(uint64_t i_date)
{
    const int i_mjd = i_date >> 24;
    struct tm tm;

    tm.tm_hour = CVT_FROM_BCD(i_date >> 16);
    tm.tm_min = CVT_FROM_BCD(i_date >> 8);
    tm.tm_sec = CVT_FROM_BCD(i_date);

    /* if all 40 bits are 1, the start is unknown */
    if (i_date == UINT64_C(0xffffffffff))
        return {};

    EITDecodeMjd(i_mjd, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    tm.tm_year -= 1900;
    tm.tm_mon--;
    tm.tm_isdst = 0;

    return tm;
}

static int EITConvertDuration(uint32_t i_duration)
{
    return CVT_FROM_BCD(i_duration >> 16) * 3600 +
        CVT_FROM_BCD(i_duration >> 8) * 60 +
        CVT_FROM_BCD(i_duration);
}

static int assetType2streamType(uint32_t assetType)
{
    int stream_type;

    switch (assetType) {
    case makeTag('h', 'e', 'v', '1'):
        stream_type = STREAM_TYPE_VIDEO_HEVC;
        break;
    case makeTag('m', 'p', '4', 'a'):
        stream_type = STREAM_TYPE_AUDIO_AAC; //STREAM_TYPE_AUDIO_AAC_LATM
        break;
    case makeTag('s', 't', 'p', 'p'):
        stream_type = STREAM_TYPE_PRIVATE_DATA;
        break;
    default:
        stream_type = 0;
        break;
    }

    return stream_type;
}

static uint8_t convertVideoComponentType(uint8_t videoResolution, uint8_t videoAspectRatio) {
    if (videoResolution > 7) {
        return 0;
    }

    uint8_t tsVideoResolutions[] = { 0x00, 0xF0, 0xD0, 0xA0, 0xC0, 0xE0, 0x90, 0x80 };
    uint8_t videoComponentType = tsVideoResolutions[videoResolution] + videoAspectRatio;

    return videoComponentType;
}

void MmtMessageHandler::onMhEit(uint8_t tableId, const MhEit* mhEit)
{
    if (mhEit->events.size() == 0) {
        return;
    }

    tsid = mhEit->tlvStreamId;

    bool pf;
    if (tableId == 0x8B) {
        //present and next program
        pf = true;
    }
    else {
        pf = false;
    }

    ts::EIT tsEit(true, pf, 0, 0, true, mhEit->serviceId, mhEit->tlvStreamId, mhEit->originalNetworkId);
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

        std::vector<uint16_t> order = {
            MhShortEventDescriptor::kDescriptorTag,
            MhExtendedEventDescriptor::kDescriptorTag,
            VideoComponentDescriptor::kDescriptorTag,
            MhContentDescriptor::kDescriptorTag,
            MhAudioComponentDescriptor::kDescriptorTag,
        };

        mhEvent->descriptors.list.sort([&order](const std::shared_ptr<MmtDescriptorBase>& a, const std::shared_ptr<MmtDescriptorBase>& b) {
            auto itA = std::find(order.begin(), order.end(), a->getDescriptorTag());
            auto itB = std::find(order.begin(), order.end(), b->getDescriptorTag());
            return itA < itB;
        });

        for (const auto& descriptor : mhEvent->descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MhShortEventDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MhShortEventDescriptor>(descriptor);
                ts::ShortEventDescriptor tsDescriptor;

                ts::UString eventName = ts::UString::FromUTF8(mmtDescriptor->eventName);
                ts::UString text = ts::UString::FromUTF8(mmtDescriptor->text);

                const ts::ByteBlock eventNameBlock(ts::ARIBCharset::B24.encoded(eventName));
                const ts::ByteBlock textBlock(ts::ARIBCharset::B24.encoded(text));

                tsDescriptor.language_code = ts::UString::FromUTF8(mmtDescriptor->language);
                tsDescriptor.event_name = ts::UString::FromUTF8((char*)eventNameBlock.data());
                tsDescriptor.text = ts::UString::FromUTF8((char*)textBlock.data());
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MhExtendedEventDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MhExtendedEventDescriptor>(descriptor);
                ts::ExtendedEventDescriptor tsDescriptor;
                tsDescriptor.descriptor_number = mmtDescriptor->descriptorNumber;
                tsDescriptor.last_descriptor_number = mmtDescriptor->lastDescriptorNumber;
                tsDescriptor.language_code = ts::UString::FromUTF8(mmtDescriptor->language);

                ts::UString text = ts::UString::FromUTF8(mmtDescriptor->textChar);
                const ts::ByteBlock textBlock(ts::ARIBCharset::B24.encoded(text));
                tsDescriptor.text = ts::UString::FromUTF8((char*)textBlock.data());

                for (auto& item : mmtDescriptor->entries) {
                    ts::ExtendedEventDescriptor::Entry entry;

                    ts::UString itemChar = ts::UString::FromUTF8(item.itemChar);
                    ts::UString itemDescriptionChar = ts::UString::FromUTF8(item.itemDescriptionChar);

                    const ts::ByteBlock itemCharBlock(ts::ARIBCharset::B24.encoded(itemChar));
                    const ts::ByteBlock itemDescriptionCharBlock(ts::ARIBCharset::B24.encoded(itemDescriptionChar));

                    entry.item = ts::UString::FromUTF8((char*)itemCharBlock.data());
                    entry.item_description = ts::UString::FromUTF8((char*)itemDescriptionCharBlock.data());

                    tsDescriptor.entries.push_back(entry);
                }
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MhAudioComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MhAudioComponentDescriptor>(descriptor);
                ts::AudioComponentDescriptor tsDescriptor;
                tsDescriptor.stream_content = mmtDescriptor->streamContent;
                tsDescriptor.component_type = mmtDescriptor->componentType;
                tsDescriptor.component_tag = mmtDescriptor->componentTag;
                tsDescriptor.stream_type = mmtDescriptor->streamType;
                tsDescriptor.simulcast_group_tag = mmtDescriptor->simulcastGroupTag;
                if (mmtDescriptor->esMultiLingualFlag) {
                    tsDescriptor.ISO_639_language_code_2 = ts::UString::FromUTF8(mmtDescriptor->language2);
                }

                tsDescriptor.main_component = mmtDescriptor->mainComponentFlag;
                tsDescriptor.quality_indicator = mmtDescriptor->qualityIndicator;
                tsDescriptor.sampling_rate = mmtDescriptor->samplingRate;
                tsDescriptor.ISO_639_language_code = ts::UString::FromUTF8(mmtDescriptor->language1);
                tsDescriptor.text = ts::UString::FromUTF8(mmtDescriptor->text);
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case VideoComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<VideoComponentDescriptor>(descriptor);
                ts::ComponentDescriptor tsDescriptor;
                tsDescriptor.stream_content = 2; //type = hevc
                tsDescriptor.component_type = convertVideoComponentType(mmtDescriptor->videoResolution, mmtDescriptor->videoAspectRatio);
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MhContentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MhContentDescriptor>(descriptor);
                ts::ContentDescriptor tsDescriptor;

                for (auto& item : mmtDescriptor->entries) {
                    ts::ContentDescriptor::Entry entry;
                    entry.content_nibble_level_1 = item.contentNibbleLevel1;
                    entry.content_nibble_level_2 = item.contentNibbleLevel2;
                    entry.user_nibble_1 = item.userNibble1;
                    entry.user_nibble_2 = item.userNibble2;
                    tsDescriptor.entries.push_back(entry);
                }
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

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void MmtMessageHandler::onMhSdt(const MhSdt* mhSdt)
{
    if (mhSdt->services.size() == 0) {
        return;
    }

    tsid = mhSdt->tlvStreamId;

    ts::SDT tsSdt(true, 0, true, mhSdt->tlvStreamId, mhSdt->originalNetworkId);
    for (auto& service : mhSdt->services) {
        ts::SDT::ServiceEntry tsService(&tsSdt);
        tsService.EITs_present = service->eitScheduleFlag;
        tsService.EITpf_present = service->eitPresentFollowingFlag;
        tsService.running_status = convertRunningStatus(service->runningStatus);
        tsService.CA_controlled = service->freeCaMode;

        for (auto& descriptor : service->descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MhServiceDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MhServiceDescriptor>(descriptor);

                const ts::ByteBlock serviceProviderName(ts::ARIBCharset::B24.encoded(
                    ts::UString::FromUTF8(mmtDescriptor->serviceProviderName)));
                const ts::ByteBlock serviceName(ts::ARIBCharset::B24.encoded(
                    ts::UString::FromUTF8(mmtDescriptor->serviceName)));

                ts::ServiceDescriptor serviceDescriptor(1,
                    ts::UString::FromUTF8((char*)serviceProviderName.data(), serviceProviderName.size()),
                    ts::UString::FromUTF8((char*)serviceName.data(), serviceName.size()));
                tsService.descs.add(duck, serviceDescriptor);
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
            packet.setPriority(true);

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void MmtMessageHandler::onPlt(const Plt* plt)
{
    if (tsid == -1)
        return;

    ts::PAT pat(0, true, tsid);
    mapService2Pid.clear();

    int i = 0;
    for (auto& item : plt->items) {
        if (item.mmtPackageIdLength != 2) {
            return;
        }

        uint16_t serviceId = swapEndian16(*(uint16_t*)item.mmtPackageIdByte.data());

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
            packet.setPriority(true);

            if (*outputFormatContext && (*outputFormatContext)->pb) {
               avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void MmtMessageHandler::onMpt(const Mpt* mpt)
{
    uint16_t serviceId;
    uint16_t pid;

    if (mpt->mmtPackageIdLength != 2) {
        return;
    }

    serviceId = swapEndian16(*(uint16_t*)mpt->mmtPackageIdByte.data());

    auto it = mapService2Pid.find(serviceId);
    if (it == mapService2Pid.end()) {
        return;
    }

    pid = it->second;

    ts::PMT tsPmt(0, true, serviceId);
    tsPmt.pcr_pid = 0x100;

    ts::CADescriptor caDescriptor(5, 0x0901);
    tsPmt.descs.add(duck, caDescriptor);

    uint8_t aribDescriptor1[] = { 0xF6, 0x04, 0x00, 0x0E, 0xE9, 0x02 };
    tsPmt.descs.add(aribDescriptor1, sizeof(aribDescriptor1));

    uint8_t aribDescriptor2[] = { 0xC1, 0x01, 0x84 };
    tsPmt.descs.add(aribDescriptor2, sizeof(aribDescriptor2));

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
                    descriptor.format_identifier = 0x48455643;
                    stream.descs.add(duck, descriptor);
                }

                tsPmt.streams[0x100 + streamIndex] = stream;
                streamIndex++;
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
            packet.setPriority(true);

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void MmtMessageHandler::onTlvNit(const TlvNit* tlvNit)
{
    ts::NIT nit(true, 0, true, 0);
    nit.network_id = tlvNit->networkId;

    for (auto& descriptor : tlvNit->descriptors) {
        switch (descriptor->descriptorTag) {
        case 0x40:
        {
            NetworkNameDescriptor* networkNameDescriptor = (NetworkNameDescriptor*)descriptor;

            const ts::ByteBlock networkNameBlock(ts::ARIBCharset::B24.encoded(
                ts::UString::FromUTF8(networkNameDescriptor->networkName)));
            ts::UString networkNameARIB = ts::UString::FromUTF8((char*)networkNameBlock.data(), networkNameBlock.size());
            ts::NetworkNameDescriptor tsDescriptor(networkNameARIB);
            nit.descs.add(duck, tsDescriptor);
            break;
        }
        }
    }

    for (auto& item : tlvNit->items) {
        ts::TransportStreamId tsid(item.tlvStreamId, item.originalNetworkId);
        nit.transports[tsid];

        for (auto& descriptor : item.descriptors) {
            switch (descriptor->descriptorTag) {
            case 0x41:
            {
                ts::ServiceListDescriptor tsDescriptor;
                ServiceListDescriptor* serviceListDescriptor = (ServiceListDescriptor*)descriptor;
                for (auto service : serviceListDescriptor->services) {
                    tsDescriptor.addService(service.serviceId, service.serviceType);
                }

                nit.transports[tsid].descs.add(duck, tsDescriptor);
                break;
            }
            case 0xCD:
            {
                break;
            }
            }
        }
    }

    ts::BinaryTable table;
    nit.serialize(duck, table);
    ts::OneShotPacketizer packetizer(duck, DVB_NIT_PID);

    for (int i = 0; i < table.sectionCount(); i++) {
        const ts::SectionPtr& section = table.sectionAt(i);

        packetizer.addSection(section);

        ts::TSPacketVector packets;
        packetizer.getPackets(packets);
        for (auto& packet : packets) {
            packet.setCC(mapCC[DVB_NIT_PID] & 0xF);
            mapCC[DVB_NIT_PID]++;

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void MmtMessageHandler::onMhTot(const MhTot* mhTot)
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

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}
