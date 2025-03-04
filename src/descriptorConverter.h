﻿#pragma once
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
#include "aribUtil.h"
#include "timeUtil.h"

constexpr uint8_t convertAudioComponentType(uint8_t componentType) {
    uint8_t audioMode = componentType & 0b00011111;

    switch (audioMode) {
    case 0b00001: // 1/0 mode (single mono) 
        return 0x01;
    case 0b00010: // 1/0＋1/0 mode (dual mono) 
        return 0x02;
    case 0b00011: // 2/0 mode (stereo) 
        return 0x03;
    case 0b00111: // 3/1 mode 
        return 0x07;
    case 0b01000: // 3/2 mode
        return 0x08;
    case 0b01001: // 3/2＋LFE mode (3/2.1 mode)
        return 0x09;
    }

    return 0;
}

constexpr uint8_t convertAudioSamplingRate(uint8_t samplingRate) {
    switch (samplingRate) {
    case 0b010: // 24 kHz
        return 0b010;
    case 0b101: // 32 kHz
        return 0b101;
    case 0b111: // 48 kHz 
        return 0b111;
    }

    return 0;
}

constexpr uint8_t convertVideoComponentType(uint8_t videoResolution, uint8_t videoAspectRatio) {
    if (videoResolution > 7) {
        return 0;
    }

    uint8_t tsVideoResolutions[] = { 0x00, 0xF0, 0xD0, 0xA0, 0xC0, 0xE0, 0x90, 0x80 };
    uint8_t videoComponentType = tsVideoResolutions[videoResolution] + videoAspectRatio;

    return videoComponentType;
}

template <typename Src>
struct DescriptorConverter;

template <>
struct DescriptorConverter<MmtTlv::MhShortEventDescriptor> {
    static std::optional<std::vector<uint8_t>> convert(const MmtTlv::MhShortEventDescriptor& mmtDescriptor) {
        const ts::ByteBlock eventNameBlock = aribEncode(mmtDescriptor.eventName);
        ts::ByteBlock textBlock = aribEncode(mmtDescriptor.text);

        if (eventNameBlock.size() > 257) {
            return std::nullopt;
        }

        size_t descriptorLength = 1 // descriptor_tag
            + 1 // descriptor_length
            + 3 // language
            + 1 // event_name_length
            + eventNameBlock.size() // event_name
            + 1 // text_length
            + textBlock.size(); // text

        // cut the text if the descriptor length is exceeded
        if (descriptorLength > 257) {
            if (descriptorLength - textBlock.size() > 257) {
                return std::nullopt;
            }

            textBlock.resize(257 - (descriptorLength - textBlock.size()));

            descriptorLength = 1 // descriptor_tag
                + 1 // descriptor_length
                + 3 // language
                + 1 // event_name_length
                + eventNameBlock.size() // event_name
                + 1 // text_length
                + textBlock.size();
        }

        std::vector<uint8_t> tsDescriptor(descriptorLength);
        tsDescriptor[0] = 0x4D; // descriptor_tag
        tsDescriptor[1] = static_cast<uint8_t>(descriptorLength) - 2; // descriptor_length

        memcpy(&tsDescriptor[2], mmtDescriptor.language, 3); // language

        tsDescriptor[5] = static_cast<uint8_t>(eventNameBlock.size()); // event_name_length
        if (eventNameBlock.size()) {
            memcpy(&tsDescriptor[6], eventNameBlock.data(), eventNameBlock.size()); // event_name
        }

        tsDescriptor[6 + eventNameBlock.size()] = static_cast<uint8_t>(textBlock.size()); // text_length
        if (textBlock.size()) {
            memcpy(&tsDescriptor[6 + eventNameBlock.size() + 1], textBlock.data(), textBlock.size()); // text
        }

        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhExtendedEventDescriptor> {
    static std::optional<std::vector<uint8_t>> convert(const MmtTlv::MhExtendedEventDescriptor& mmtDescriptor) {
        const ts::ByteBlock textBlock = aribEncode(mmtDescriptor.textChar);

        if (textBlock.size() > 255) {
            return std::nullopt;
        }

        std::vector<ts::ByteBlock> aribItemDescriptionChars;
        std::vector<ts::ByteBlock> aribItemChars;

        size_t descriptorLength = 1 // descriptor_tag
            + 1 // descriptor_length
            + 1 // descriptorNumber | lastDescriptorNumber
            + 3 // language
            + 1; // length_of_items

        size_t itemsLength = 0;
        for (const auto& item : mmtDescriptor.entries) {
            const ts::ByteBlock itemDescriptionCharBlock = aribEncode(item.itemDescriptionChar);
            const ts::ByteBlock itemCharBlock = aribEncode(item.itemChar);

            if (itemDescriptionCharBlock.size() > 255) {
                return std::nullopt;
            }

            if (itemCharBlock.size() > 255) {
                return std::nullopt;
            }

            aribItemDescriptionChars.push_back(itemDescriptionCharBlock);
            aribItemChars.push_back(itemCharBlock);

            itemsLength += 1 + itemDescriptionCharBlock.size() + 1 + itemCharBlock.size();
        }

        if (itemsLength > 255) {
            return std::nullopt;
        }

        descriptorLength += itemsLength
            + 1 // text_length
            + textBlock.size(); // text

        if (descriptorLength > 255) {
            return std::nullopt;
        }

        std::vector<uint8_t> tsDescriptor(descriptorLength);
        tsDescriptor[0] = 0x4E;
        tsDescriptor[1] = static_cast<uint8_t>(descriptorLength) - 2;
        tsDescriptor[2] = (mmtDescriptor.descriptorNumber & 0b1111) << 4 | (mmtDescriptor.lastDescriptorNumber & 0b1111);
        memcpy(&tsDescriptor[3], mmtDescriptor.language, 3);

        tsDescriptor[6] = static_cast<uint8_t>(itemsLength);

        size_t pos = 0;
        for (size_t i = 0; i < mmtDescriptor.entries.size(); ++i) {
            tsDescriptor[7 + pos] = static_cast<uint8_t>(aribItemDescriptionChars[i].size());
            pos++;

            if (aribItemDescriptionChars[i].size()) {
                memcpy(&tsDescriptor[7 + pos], aribItemDescriptionChars[i].data(), aribItemDescriptionChars[i].size());
                pos += aribItemDescriptionChars[i].size();
            }

            tsDescriptor[7 + pos] = static_cast<uint8_t>(aribItemChars[i].size());
            pos++;

            if (aribItemChars[i].size()) {
                memcpy(&tsDescriptor[7 + pos], aribItemChars[i].data(), aribItemChars[i].size());
                pos += aribItemChars[i].size();
            }

            i++;
        }

        tsDescriptor[7 + pos] = static_cast<uint8_t>(textBlock.size());
        pos++;

        if (textBlock.size()) {
            memcpy(&tsDescriptor[7 + pos], textBlock.data(), textBlock.size());
        }

        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhAudioComponentDescriptor> {
    static ts::AudioComponentDescriptor convert(const MmtTlv::MhAudioComponentDescriptor& mmtDescriptor) {
        ts::AudioComponentDescriptor tsDescriptor;
        tsDescriptor.stream_content = 2; // audio
        tsDescriptor.component_type = convertAudioComponentType(mmtDescriptor.componentType);
        tsDescriptor.component_tag = static_cast<uint8_t>(mmtDescriptor.componentTag);
        tsDescriptor.stream_type = 0x0F; // ISO/IEC13818-7 audio
        tsDescriptor.simulcast_group_tag = mmtDescriptor.simulcastGroupTag;
        if (mmtDescriptor.esMultiLingualFlag) {
            tsDescriptor.ISO_639_language_code_2 = ts::UString::FromUTF8(mmtDescriptor.language2);
        }

        tsDescriptor.main_component = mmtDescriptor.mainComponentFlag;
        tsDescriptor.quality_indicator = mmtDescriptor.qualityIndicator;
        tsDescriptor.sampling_rate = convertAudioSamplingRate(mmtDescriptor.samplingRate);
        tsDescriptor.ISO_639_language_code = ts::UString::FromUTF8(mmtDescriptor.language1);

        const ts::ByteBlock textBlock = aribEncode(mmtDescriptor.text);
        tsDescriptor.text = ts::UString::FromUTF8(reinterpret_cast<const char*>(textBlock.data()), textBlock.size());
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::VideoComponentDescriptor> {
    static ts::ComponentDescriptor convert(const MmtTlv::VideoComponentDescriptor& mmtDescriptor) {
        ts::ComponentDescriptor tsDescriptor;
        tsDescriptor.stream_content = 1; // video
        tsDescriptor.component_type = convertVideoComponentType(mmtDescriptor.videoResolution, mmtDescriptor.videoAspectRatio);
        tsDescriptor.language_code = ts::UString::FromUTF8(mmtDescriptor.language);

        const ts::ByteBlock textBlock = aribEncode(mmtDescriptor.text);
        tsDescriptor.text = ts::UString::FromUTF8(reinterpret_cast<const char*>(textBlock.data()), textBlock.size());
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhContentDescriptor> {
    static ts::ContentDescriptor convert(const MmtTlv::MhContentDescriptor& mmtDescriptor) {
        ts::ContentDescriptor tsDescriptor;

        for (auto& item : mmtDescriptor.entries) {
            ts::ContentDescriptor::Entry entry;
            entry.content_nibble_level_1 = item.contentNibbleLevel1;
            entry.content_nibble_level_2 = item.contentNibbleLevel2;
            entry.user_nibble_1 = item.userNibble1;
            entry.user_nibble_2 = item.userNibble2;
            tsDescriptor.entries.push_back(entry);
        }
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhLinkageDescriptor> {
    static ts::LinkageDescriptor convert(const MmtTlv::MhLinkageDescriptor& mmtDescriptor) {
        ts::LinkageDescriptor tsDescriptor(mmtDescriptor.tlvStreamId, mmtDescriptor.originalNetworkId, mmtDescriptor.serviceId, mmtDescriptor.linkageType);
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhEventGroupDescriptor> {
    static ts::EventGroupDescriptor convert(const MmtTlv::MhEventGroupDescriptor& mmtDescriptor) {
        ts::EventGroupDescriptor tsDescriptor;
        tsDescriptor.group_type = mmtDescriptor.groupType;

        for (const auto& event : mmtDescriptor.events) {
            ts::EventGroupDescriptor::ActualEvent actualEvent;
            actualEvent.service_id = event.serviceId;
            actualEvent.event_id = event.eventId;
            tsDescriptor.actual_events.push_back(actualEvent);
        }

        for (const auto& otherNetworkEvent : mmtDescriptor.otherNetworkEvents) {
            ts::EventGroupDescriptor::OtherEvent otherEvent;
            otherEvent.original_network_id = otherNetworkEvent.originalNetworkId;
            otherEvent.transport_stream_id = otherNetworkEvent.tlvStreamId;
            otherEvent.service_id = otherNetworkEvent.serviceId;
            otherEvent.event_id = otherNetworkEvent.eventId;
            tsDescriptor.other_events.push_back(otherEvent);
        }

        tsDescriptor.private_data.resize(mmtDescriptor.privateDataByte.size());
        if (mmtDescriptor.privateDataByte.size()) {
            memcpy(tsDescriptor.private_data.data(), mmtDescriptor.privateDataByte.data(), mmtDescriptor.privateDataByte.size());
        }
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhParentalRatingDescriptor> {
    static ts::ParentalRatingDescriptor convert(const MmtTlv::MhParentalRatingDescriptor& mmtDescriptor) {
        ts::ParentalRatingDescriptor tsDescriptor;

        for (const auto& entry : mmtDescriptor.entries) {
            ts::UString countryCode = ts::UString::FromUTF8(entry.countryCode);
            ts::ParentalRatingDescriptor::Entry tsEntry(countryCode, entry.rating);
            tsDescriptor.entries.push_back(tsEntry);
        }

        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhSeriesDescriptor> {
    static ts::SeriesDescriptor convert(const MmtTlv::MhSeriesDescriptor& mmtDescriptor) {
        ts::SeriesDescriptor tsDescriptor;
        tsDescriptor.series_id = mmtDescriptor.seriesId;
        tsDescriptor.repeat_label = mmtDescriptor.repeatLabel;
        tsDescriptor.program_pattern = mmtDescriptor.programPattern;

        if (mmtDescriptor.expireDateValidFlag) {
            struct tm tm;
            EITDecodeMjd(mmtDescriptor.expireDate, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);

            try {
                tsDescriptor.expire_date = ts::Time(tm.tm_year, tm.tm_mon, tm.tm_mday, 0, 0);
            }
            catch (const ts::Time::TimeError&) {
                return {};
            }
        }

        tsDescriptor.episode_number = mmtDescriptor.episodeNumber;
        tsDescriptor.last_episode_number = mmtDescriptor.lastEpisodeNumber;

        const ts::ByteBlock seriesNameBlock = aribEncode(mmtDescriptor.seriesNameChar);
        tsDescriptor.series_name = ts::UString::FromUTF8(reinterpret_cast<const char*>(seriesNameBlock.data()), seriesNameBlock.size());

        return tsDescriptor;
    }
};


template <>
struct DescriptorConverter<MmtTlv::ContentCopyControlDescriptor> {
    static ts::DigitalCopyControlDescriptor convert(const MmtTlv::ContentCopyControlDescriptor& mmtDescriptor) {
        ts::DigitalCopyControlDescriptor tsDescriptor;
        tsDescriptor.digital_recording_control_data = mmtDescriptor.digitalRecordingControlData;

        if (mmtDescriptor.maximumBitrateFlag) {
            tsDescriptor.maximum_bitrate = mmtDescriptor.maximumBitrate;
        }

        for (const auto& component : mmtDescriptor.components) {
            ts::DigitalCopyControlDescriptor::Component tsComponent;
            tsComponent.component_tag = static_cast<uint8_t>(component.componentTag);
            tsComponent.digital_recording_control_data = component.digitalRecordingControlData;
            if (component.maximumBitrateFlag) {
                tsComponent.maximum_bitrate = component.maximumBitrate;
            }
            tsDescriptor.components.push_back(tsComponent);
        }

        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MultimediaServiceInformationDescriptor> {
    static ts::DataContentDescriptor convert(const MmtTlv::MultimediaServiceInformationDescriptor& mmtDescriptor) {
        ts::DataContentDescriptor tsDescriptor;
        tsDescriptor.data_component_id = mmtDescriptor.dataComponentId;

        if (mmtDescriptor.dataComponentId == 0x0020) {
            tsDescriptor.ISO_639_language_code = ts::UString::FromUTF8(mmtDescriptor.language);

            const ts::ByteBlock textBlock = aribEncode(mmtDescriptor.text);
            tsDescriptor.text = ts::UString::FromUTF8(reinterpret_cast<const char*>(textBlock.data()), textBlock.size());
        }

        tsDescriptor.selector_bytes.resize(mmtDescriptor.selectorByte.size());
        if (mmtDescriptor.selectorByte.size()) {
            memcpy(tsDescriptor.selector_bytes.data(), mmtDescriptor.selectorByte.data(), mmtDescriptor.selectorByte.size());
        }

        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhServiceDescriptor> {
    static ts::ServiceDescriptor convert(const MmtTlv::MhServiceDescriptor& mmtDescriptor) {
        const ts::ByteBlock serviceProviderName = aribEncode(mmtDescriptor.serviceProviderName);
        const ts::ByteBlock serviceName(ts::ARIBCharset::B24.encoded(
            ts::UString::FromUTF8(mmtDescriptor.serviceName.data(), mmtDescriptor.serviceName.size())));

        ts::ServiceDescriptor tsDescriptor(1,
            ts::UString::FromUTF8((char*)serviceProviderName.data(), serviceProviderName.size()),
            ts::UString::FromUTF8((char*)serviceName.data(), serviceName.size()));
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhLogoTransmissionDescriptor> {
    static ts::LogoTransmissionDescriptor convert(const MmtTlv::MhLogoTransmissionDescriptor& mmtDescriptor) {
        ts::LogoTransmissionDescriptor tsDescriptor;
        tsDescriptor.logo_transmission_type = mmtDescriptor.logoTransmissionType;
        if (mmtDescriptor.logoTransmissionType == 0x01) {
            tsDescriptor.logo_id = mmtDescriptor.logoId;
            tsDescriptor.logo_version = mmtDescriptor.logoVersion;
            tsDescriptor.download_data_id = mmtDescriptor.downloadDataId;
        }
        else if (mmtDescriptor.logoTransmissionType == 0x02) {
            tsDescriptor.logo_id = mmtDescriptor.logoId;
        }
        else if (mmtDescriptor.logoTransmissionType == 0x03) {
            const ts::ByteBlock logoCharBlock = aribEncode(mmtDescriptor.logoChar);
            tsDescriptor.logo_char = ts::UString::FromUTF8(reinterpret_cast<const char*>(logoCharBlock.data()), logoCharBlock.size());
        }
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::MhStreamIdentificationDescriptor> {
    static ts::StreamIdentifierDescriptor convert(const MmtTlv::MhStreamIdentificationDescriptor& mmtDescriptor) {
        ts::StreamIdentifierDescriptor tsDescriptor(static_cast<uint8_t>(mmtDescriptor.componentTag));
        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::AccessControlDescriptor> {
    static ts::ISDBAccessControlDescriptor convert(const MmtTlv::AccessControlDescriptor& mmtDescriptor) {
        ts::ISDBAccessControlDescriptor tsDescriptor;
        tsDescriptor.CA_system_id = mmtDescriptor.caSystemId;
        tsDescriptor.pid = 0x200; // Not implemented

        tsDescriptor.private_data.resize(mmtDescriptor.privateData.size());
        if (mmtDescriptor.privateData.size()) {
            memcpy(tsDescriptor.private_data.data(), mmtDescriptor.privateData.data(), mmtDescriptor.privateData.size());
        }

        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::NetworkNameDescriptor> {
    static ts::NetworkNameDescriptor convert(const MmtTlv::NetworkNameDescriptor& mmtDescriptor) {
        const ts::ByteBlock networkNameBlock = aribEncode(mmtDescriptor.networkName);
        ts::UString networkNameARIB = ts::UString::FromUTF8((char*)networkNameBlock.data(), networkNameBlock.size());
        ts::NetworkNameDescriptor tsDescriptor(networkNameARIB);

        return tsDescriptor;
    }
};

template <>
struct DescriptorConverter<MmtTlv::ServiceListDescriptor> {
    static ts::ServiceListDescriptor convert(const MmtTlv::ServiceListDescriptor& mmtDescriptor) {
        ts::ServiceListDescriptor tsDescriptor;
        for (auto const& service : mmtDescriptor.services) {
            tsDescriptor.addService(service.serviceId, service.serviceType);
        }
        return tsDescriptor;
    }
};

