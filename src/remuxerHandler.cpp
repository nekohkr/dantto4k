#include "remuxerHandler.h"
#include "mhEit.h"
#include "mhExtendedEventDescriptor.h"
#include "mhShortEventDescriptor.h"
#include "mhContentDescriptor.h"
#include "mhAudioComponentDescriptor.h"
#include "videoComponentDescriptor.h"
#include "mhLinkageDescriptor.h"
#include "mhEventGroupDescriptor.h"
#include "mhParentalRatingDescriptor.h"
#include "mhSeriesDescriptor.h"
#include "mhLogoTransmissionDescriptor.h"
#include "mhStreamIdentificationDescriptor.h"
#include "networkNameDescriptor.h"
#include "serviceListDescriptor.h"
#include "contentCopyControlDescriptor.h"
#include "multimediaServiceInformationDescriptor.h"
#include "accessControlDescriptor.h"

#include "mhSdt.h"
#include "mhServiceDescriptor.h"
#include "plt.h"
#include "nit.h"
#include "mpt.h"
#include "mhTot.h"
#include "mhCdt.h"

#include "adtsConverter.h"

#include "mmtTlvDemuxer.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include "mfuDataProcessorBase.h"

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

void EITDecodeMjd(int i_mjd, int* p_y, int* p_m, int* p_d)
{
    const int yp = (int)(((double)i_mjd - 15078.2) / 365.25);
    const int mp = (int)(((double)i_mjd - 14956.1 - (int)(yp * 365.25)) / 30.6001);
    const int c = (mp == 14 || mp == 15) ? 1 : 0;

    *p_y = 1900 + yp + c * 1;
    *p_m = mp - 1 - c * 12;
    *p_d = i_mjd - 14956 - (int)(yp * 365.25) - (int)(mp * 30.6001);
}

#define CVT_FROM_BCD(v) ((((v) >> 4)&0xf)*10 + ((v)&0xf))
struct tm EITConvertStartTime(uint64_t i_date)
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

int EITConvertDuration(uint32_t i_duration)
{
    return CVT_FROM_BCD(i_duration >> 16) * 3600 +
        CVT_FROM_BCD(i_duration >> 8) * 60 +
        CVT_FROM_BCD(i_duration);
}

int assetType2streamType(uint32_t assetType)
{
    int stream_type = 0;
    switch (assetType) {
    case MmtTlv::makeAssetType('h', 'e', 'v', '1'):
        stream_type = STREAM_TYPE_VIDEO_HEVC;
        break;
    case MmtTlv::makeAssetType('m', 'p', '4', 'a'):
        stream_type = STREAM_TYPE_AUDIO_AAC; // STREAM_TYPE_AUDIO_AAC_LATM
        break;
    case MmtTlv::makeAssetType('s', 't', 'p', 'p'):
        stream_type = STREAM_TYPE_PRIVATE_DATA;
        break;
    }

    return stream_type;
}

uint8_t convertVideoComponentType(uint8_t videoResolution, uint8_t videoAspectRatio) {
    if (videoResolution > 7) {
        return 0;
    }

    uint8_t tsVideoResolutions[] = { 0x00, 0xF0, 0xD0, 0xA0, 0xC0, 0xE0, 0x90, 0x80 };
    uint8_t videoComponentType = tsVideoResolutions[videoResolution] + videoAspectRatio;

    return videoComponentType;
}

} // anonymous namespace

void RemuxerHandler::onVideoData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData)
{
    writeStream(mmtStream, mfuData, mfuData->data);
}

void RemuxerHandler::onAudioData(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData)
{
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

void RemuxerHandler::writeStream(const std::shared_ptr<MmtTlv::MmtStream> mmtStream, const std::shared_ptr<struct MmtTlv::MfuData>& mfuData, std::vector<uint8_t> streamData)
{
    AVStream* outStream = (*outputFormatContext)->streams[mfuData->streamIndex];
    AVRational timeBase = { mmtStream->timeBase.num, mmtStream->timeBase.den };

    uint8_t* data = (uint8_t*)malloc(streamData.size() + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(data, streamData.data(), streamData.size());
    memset(data + streamData.size(), 0, AV_INPUT_BUFFER_PADDING_SIZE);

    AVBufferRef* buf = av_buffer_create(data, streamData.size() + AV_INPUT_BUFFER_PADDING_SIZE, [](void* opaque, uint8_t* data) {
        free(data);
        }, NULL, 0);

    AVPacket* packet = av_packet_alloc();
    packet->buf = buf;
    packet->data = buf->data;
    packet->pts = mfuData->pts;
    packet->dts = mfuData->dts;
    packet->stream_index = mfuData->streamIndex;
    packet->flags = mfuData->flags;
    packet->pos = -1;
    packet->duration = 0;
    packet->size = buf->size - AV_INPUT_BUFFER_PADDING_SIZE;

    av_packet_rescale_ts(packet, timeBase, outStream->time_base);
    if (av_interleaved_write_frame((*outputFormatContext), packet) < 0) {
        std::cerr << "Error muxing packet." << std::endl;
        return;
    }

    av_packet_unref(packet);
}

void RemuxerHandler::onMhEit(const std::shared_ptr<MmtTlv::MhEit>& mhEit)
{
    tsid = mhEit->tlvStreamId;

    ts::EIT tsEit(true, mhEit->isPF(), 0, 0, true, mhEit->serviceId, mhEit->tlvStreamId, mhEit->originalNetworkId);
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
            MmtTlv::MhShortEventDescriptor::kDescriptorTag,
            MmtTlv::MhExtendedEventDescriptor::kDescriptorTag,
            MmtTlv::VideoComponentDescriptor::kDescriptorTag,
            MmtTlv::MhContentDescriptor::kDescriptorTag,
            MmtTlv::ContentCopyControlDescriptor::kDescriptorTag,
            MmtTlv::MhAudioComponentDescriptor::kDescriptorTag,
            MmtTlv::MultimediaServiceInformationDescriptor::kDescriptorTag,
            MmtTlv::MhEventGroupDescriptor::kDescriptorTag,
        };

        mhEvent->descriptors.list.sort([&order](const std::shared_ptr<MmtTlv::MmtDescriptorBase>& a, const std::shared_ptr<MmtTlv::MmtDescriptorBase>& b) {
            auto itA = std::find(order.begin(), order.end(), a->getDescriptorTag());
            auto itB = std::find(order.begin(), order.end(), b->getDescriptorTag());
            return itA < itB;
        });

        for (const auto& descriptor : mhEvent->descriptors.list) {
            switch (descriptor->getDescriptorTag()) {
            case MmtTlv::MhShortEventDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhShortEventDescriptor>(descriptor);
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
            case MmtTlv::MhExtendedEventDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhExtendedEventDescriptor>(descriptor);
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
            case MmtTlv::MhAudioComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhAudioComponentDescriptor>(descriptor);
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
            case MmtTlv::VideoComponentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::VideoComponentDescriptor>(descriptor);
                ts::ComponentDescriptor tsDescriptor;
                tsDescriptor.stream_content = 2; //type = hevc
                tsDescriptor.component_type = convertVideoComponentType(mmtDescriptor->videoResolution, mmtDescriptor->videoAspectRatio);
                tsDescriptor.language_code = ts::UString::FromUTF8(mmtDescriptor->language);
                tsDescriptor.text = ts::UString::FromUTF8(mmtDescriptor->text);
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhContentDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhContentDescriptor>(descriptor);
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
            case MmtTlv::MhLinkageDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhLinkageDescriptor>(descriptor);
                ts::LinkageDescriptor tsDescriptor(mmtDescriptor->tlvStreamId, mmtDescriptor->originalNetworkId, mmtDescriptor->serviceId, mmtDescriptor->linkageType);
                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhEventGroupDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhEventGroupDescriptor>(descriptor);
                ts::EventGroupDescriptor tsDescriptor;
                tsDescriptor.group_type = mmtDescriptor->groupType;

                for (const auto& event : mmtDescriptor->events) {
                    ts::EventGroupDescriptor::ActualEvent actualEvent;
                    actualEvent.service_id = event.serviceId;
                    actualEvent.event_id = event.eventId;
                    tsDescriptor.actual_events.push_back(actualEvent);
                }

                for (const auto& otherNetworkEvent : mmtDescriptor->otherNetworkEvents) {
                    ts::EventGroupDescriptor::OtherEvent otherEvent;
                    otherEvent.original_network_id = otherNetworkEvent.originalNetworkId;
                    otherEvent.transport_stream_id = otherNetworkEvent.tlvStreamId;
                    otherEvent.service_id = otherNetworkEvent.serviceId;
                    otherEvent.event_id = otherNetworkEvent.eventId;
                    tsDescriptor.other_events.push_back(otherEvent);
                }

                tsDescriptor.private_data.resize(mmtDescriptor->privateDataByte.size());
                memcpy(tsDescriptor.private_data.data(), mmtDescriptor->privateDataByte.data(), mmtDescriptor->privateDataByte.size());

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhParentalRatingDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhParentalRatingDescriptor>(descriptor);
                ts::ParentalRatingDescriptor tsDescriptor;

                for (const auto& entry : mmtDescriptor->entries) {
                    ts::UString countryCode = ts::UString::FromUTF8(entry.countryCode);
                    ts::ParentalRatingDescriptor::Entry tsEntry(countryCode, entry.rating);
                    tsDescriptor.entries.push_back(tsEntry);
                }

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MhSeriesDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhSeriesDescriptor>(descriptor);
                ts::SeriesDescriptor tsDescriptor;
                tsDescriptor.series_id = mmtDescriptor->seriesId;
                tsDescriptor.repeat_label = mmtDescriptor->repeatLabel;
                tsDescriptor.program_pattern = mmtDescriptor->programPattern;

                if (mmtDescriptor->expireDateValidFlag) {
                    struct tm tm;
                    EITDecodeMjd(mmtDescriptor->expireDate, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
                    tm.tm_year -= 1900;
                    tm.tm_mon--;
                    tm.tm_isdst = 0;

                    try {
                        tsDescriptor.expire_date = ts::Time(tm.tm_year, tm.tm_mon, tm.tm_mday, 0, 0);
                    }
                    catch (ts::Time::TimeError) {
                        return;
                    }
                }

                tsDescriptor.episode_number = mmtDescriptor->episodeNumber;
                tsDescriptor.last_episode_number = mmtDescriptor->lastEpisodeNumber;

                ts::UString seriesName = ts::UString::FromUTF8(mmtDescriptor->seriesNameChar);
                tsDescriptor.series_name = seriesName;

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::ContentCopyControlDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::ContentCopyControlDescriptor>(descriptor);
                ts::DigitalCopyControlDescriptor tsDescriptor;
                tsDescriptor.digital_recording_control_data = mmtDescriptor->digitalRecordingControlData;

                if (mmtDescriptor->maximumBitrateFlag) {
                    tsDescriptor.maximum_bitrate = mmtDescriptor->maximumBitrate;
                }

                for (const auto& component : mmtDescriptor->components) {
                    ts::DigitalCopyControlDescriptor::Component tsComponent;
                    tsComponent.component_tag = component.componentTag;
                    tsComponent.digital_recording_control_data = component.digitalRecordingControlData;
                    if (component.maximumBitrateFlag) {
                        tsComponent.maximum_bitrate = component.maximumBitrate;
                    }
                    tsDescriptor.components.push_back(tsComponent);
                }

                tsEvent.descs.add(duck, tsDescriptor);
                break;
            }
            case MmtTlv::MultimediaServiceInformationDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MultimediaServiceInformationDescriptor>(descriptor);
                ts::DataContentDescriptor tsDescriptor;
                tsDescriptor.data_component_id = mmtDescriptor->dataComponentId;

                if (mmtDescriptor->dataComponentId == 0x0020) {
                    tsDescriptor.ISO_639_language_code = ts::UString::FromUTF8(mmtDescriptor->language);
                    tsDescriptor.text = ts::UString::FromUTF8(mmtDescriptor->text);
                }

                tsDescriptor.selector_bytes.resize(mmtDescriptor->selectorByte.size());
                memcpy(tsDescriptor.selector_bytes.data(), mmtDescriptor->selectorByte.data(), mmtDescriptor->selectorByte.size());

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

void RemuxerHandler::onMhSdt(const std::shared_ptr<MmtTlv::MhSdt>& mhSdt)
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
            case MmtTlv::MhServiceDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhServiceDescriptor>(descriptor);

                const ts::ByteBlock serviceProviderName(ts::ARIBCharset::B24.encoded(
                    ts::UString::FromUTF8(mmtDescriptor->serviceProviderName)));
                const ts::ByteBlock serviceName(ts::ARIBCharset::B24.encoded(
                    ts::UString::FromUTF8(mmtDescriptor->serviceName)));

                ts::ServiceDescriptor serviceDescriptor(1,
                    ts::UString::FromUTF8((char*)serviceProviderName.data(), serviceProviderName.size()),
                    ts::UString::FromUTF8((char*)serviceName.data(), serviceName.size()));
                tsService.descs.add(duck, serviceDescriptor);
                break;
            }
            case MmtTlv::MhLogoTransmissionDescriptor::kDescriptorTag:
            {
                auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::MhLogoTransmissionDescriptor>(descriptor);
                ts::LogoTransmissionDescriptor tsDescriptor;
                tsDescriptor.logo_transmission_type = mmtDescriptor->logoTransmissionType;
                if (mmtDescriptor->logoTransmissionType == 0x01) {
                    tsDescriptor.logo_id = mmtDescriptor->logoId;
                    tsDescriptor.logo_version = mmtDescriptor->logoVersion;
                    tsDescriptor.download_data_id = mmtDescriptor->downloadDataId;
                } else if (mmtDescriptor->logoTransmissionType == 0x02) {
                    tsDescriptor.logo_id = mmtDescriptor->logoId;
                }
                else if (mmtDescriptor->logoTransmissionType == 0x03) {
                    ts::UString logoChar = ts::UString::FromUTF8(mmtDescriptor->logoChar);
                    tsDescriptor.logo_char = logoChar;
                }
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
            packet.setPriority(true);

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void RemuxerHandler::onPlt(const std::shared_ptr<MmtTlv::Plt>& plt)
{
    if (tsid == -1)
        return;

    ts::PAT pat(0, true, tsid);
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
            packet.setPriority(true);

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
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

    ts::PMT tsPmt(0, true, serviceId);
    tsPmt.pcr_pid = 0x100;

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
                        ts::StreamIdentifierDescriptor tsDescriptor(mmtDescriptor->componentTag);
                        stream.descs.add(duck, tsDescriptor);
                        break;
                    }
                    }
                }

                const auto& mmtStream = demuxer.mapStreamByStreamIdx[streamIndex];
                if (mmtStream->componentTag == -1) {
                    streamIndex++;
                    continue;
                }

                tsPmt.streams[mmtStream->getTsPid()] = stream;
                streamIndex++;
            }
        }
    }

    for (const auto& descriptor : mpt->descriptors.list) {
        switch (descriptor->getDescriptorTag()) {
        case MmtTlv::AccessControlDescriptor::kDescriptorTag:
        {
            auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::AccessControlDescriptor>(descriptor);
            ts::ISDBAccessControlDescriptor tsDescriptor;
            tsDescriptor.CA_system_id = mmtDescriptor->caSystemId;
            tsDescriptor.pid = 0x200; // Not implemented
            tsDescriptor.private_data.resize(mmtDescriptor->privateData.size());

            memcpy(tsDescriptor.private_data.data(), mmtDescriptor->privateData.data(), mmtDescriptor->privateData.size());

            tsPmt.descs.add(duck, tsDescriptor);
            break;
        }
        
        case MmtTlv::ContentCopyControlDescriptor::kDescriptorTag:
        {
            auto mmtDescriptor = std::dynamic_pointer_cast<MmtTlv::ContentCopyControlDescriptor>(descriptor);
            ts::DigitalCopyControlDescriptor tsDescriptor;
            tsDescriptor.digital_recording_control_data = mmtDescriptor->digitalRecordingControlData;

            if (mmtDescriptor->maximumBitrateFlag) {
                tsDescriptor.maximum_bitrate = mmtDescriptor->maximumBitrate;
            }

            for (const auto& component : mmtDescriptor->components) {
                ts::DigitalCopyControlDescriptor::Component tsComponent;
                tsComponent.component_tag = component.componentTag;
                tsComponent.digital_recording_control_data = component.digitalRecordingControlData;
                if (component.maximumBitrateFlag) {
                    tsComponent.maximum_bitrate = component.maximumBitrate;
                }
                tsDescriptor.components.push_back(tsComponent);
            }

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
            packet.setPriority(true);

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
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

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void RemuxerHandler::onMhCdt(const std::shared_ptr<MmtTlv::MhCdt>& mhCdt)
{
    ts::CDT cdt;
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

            if (*outputFormatContext && (*outputFormatContext)->pb) {
                avio_write((*outputFormatContext)->pb, packet.b, packet.getHeaderSize() + packet.getPayloadSize());
            }
        }
    }
}

void RemuxerHandler::onNit(const std::shared_ptr<MmtTlv::Nit>& nit)
{
    ts::NIT tsNit(true, 0, true, 0);
    tsNit.network_id = nit->networkId;

    for (const auto& descriptor : nit->descriptors.list) {
        switch (descriptor->getDescriptorTag()) {
        case MmtTlv::NetworkNameDescriptor::kDescriptorTag:
        {
            auto networkNameDescriptor = std::dynamic_pointer_cast<MmtTlv::NetworkNameDescriptor>(descriptor);

            const ts::ByteBlock networkNameBlock(ts::ARIBCharset::B24.encoded(
                ts::UString::FromUTF8(networkNameDescriptor->networkName)));
            ts::UString networkNameARIB = ts::UString::FromUTF8((char*)networkNameBlock.data(), networkNameBlock.size());
            ts::NetworkNameDescriptor tsDescriptor(networkNameARIB);
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
                auto serviceListDescriptor = std::dynamic_pointer_cast<MmtTlv::ServiceListDescriptor>(descriptor);
                ts::ServiceListDescriptor tsDescriptor;
                for (auto service : serviceListDescriptor->services) {
                    tsDescriptor.addService(service.serviceId, service.serviceType);
                }

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

void RemuxerHandler::onStreamsChanged()
{
    if (demuxer.mapStream.size() == 0) {
        return;
    }

    if (*outputFormatContext) {
        av_write_trailer(*outputFormatContext);
        avformat_free_context(*outputFormatContext);
    }

    avformat_alloc_output_context2(outputFormatContext, nullptr, "mpegts", nullptr);
    if (!*outputFormatContext) {
        std::cerr << "Could not create output context." << std::endl;
        return;
    }

    (*outputFormatContext)->pb = *avioContext;
    (*outputFormatContext)->flags |= AVFMT_FLAG_CUSTOM_IO;

    streamCount = 0;
    for (const auto& mmtStream : demuxer.mapStream) {
        AVStream* outStream = avformat_new_stream(*outputFormatContext, nullptr);

        switch (mmtStream.second->assetType) {
        case MmtTlv::makeAssetType('h', 'e', 'v', '1'):
            outStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
            outStream->codecpar->codec_id = AV_CODEC_ID_HEVC;
            break;
        case MmtTlv::makeAssetType('m', 'p', '4', 'a'):
            outStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
            outStream->codecpar->codec_id = AV_CODEC_ID_AAC; // AV_CODEC_ID_AAC_LATM
            outStream->codecpar->sample_rate = 48000;
            break;
        case MmtTlv::makeAssetType('s', 't', 'p', 'p'):
            outStream->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
            outStream->codecpar->codec_id = AV_CODEC_ID_TTML;
            break;
        }
        outStream->codecpar->codec_tag = 0;
        streamCount++;
    }

    if (avformat_write_header(*outputFormatContext, nullptr) < 0) {
        std::cerr << "Could not open output file." << std::endl;
        avformat_free_context(*outputFormatContext);
        return;
    }
}
