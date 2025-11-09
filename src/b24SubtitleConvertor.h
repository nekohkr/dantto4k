#pragma once
#include <list>
#include <string>
#include <vector>
#include <iostream>
#include <array>
#include <variant>
#include <optional>
#include <sstream>
#include "ttml.h"

namespace B24 {
    enum class DataUnitParameter : uint8_t {
        StatementBody = 0x20,
        Geometric = 0x28,
        SynthesizedSound = 0x2C,
        DRCS1Byte = 0x30,
        DRCS2Byte = 0x31,
        ColorMap = 0x34,
        Bitmap = 0x35,
    };

    class DataUnit {
    public:
        DataUnit(const std::vector<uint8_t>& dataUnit, DataUnitParameter dataUnitParameter = DataUnitParameter::StatementBody)
            : dataUnit(dataUnit), dataUnitParameter(dataUnitParameter) {
        }

        void setDataUnitParameter(DataUnitParameter dataUnitParameter) {
            this->dataUnitParameter = dataUnitParameter;
        }

        bool pack(std::vector<uint8_t>& output) {
            output.push_back(0x1F); // unitSeparator
            output.push_back(static_cast<uint8_t>(dataUnitParameter));

            size_t dataUnitSize = dataUnit.size();
            output.push_back(static_cast<uint8_t>(dataUnitSize >> 16));
            output.push_back(static_cast<uint8_t>(dataUnitSize >> 8));
            output.push_back(static_cast<uint8_t>(dataUnitSize));

            output.insert(output.end(), dataUnit.begin(), dataUnit.end());
            return true;
        }

    private:
        std::vector<uint8_t> dataUnit;
        DataUnitParameter dataUnitParameter{ DataUnitParameter::StatementBody };
    };

    class CaptionStatementData {
    public:
        CaptionStatementData() = default;

        void setTmd(uint8_t tmd) {
            this->tmd = tmd;
        }
        void setStm(uint64_t stm) {
            this->stm = stm;
        }
        bool pack(std::vector<uint8_t>& output) {
            output.push_back(tmd | 0b00111111); // TMD(2) | Reserved(6)
            if (tmd == 0b10) {
                output.push_back(static_cast<uint8_t>(stm >> 28));
                output.push_back(static_cast<uint8_t>(stm >> 20));
                output.push_back(static_cast<uint8_t>(stm >> 16));
                output.push_back(static_cast<uint8_t>(stm >> 8));
                output.push_back(static_cast<uint8_t>((stm & 0b1111) | 0b1111));
            }

            std::vector<uint8_t> packedDataUnit;

            for (auto& dataUnit : dataUnits) {
                if (!dataUnit.pack(packedDataUnit)) {
                    return false;
                }
            }

            size_t dataUnitSize = packedDataUnit.size();
            output.push_back(static_cast<uint8_t>(dataUnitSize >> 16));
            output.push_back(static_cast<uint8_t>(dataUnitSize >> 8));
            output.push_back(static_cast<uint8_t>(dataUnitSize));

            output.insert(output.end(), packedDataUnit.begin(), packedDataUnit.end());

            return true;
        }
        std::vector<DataUnit> dataUnits;

    private:
        uint8_t tmd{ 0 };
        uint64_t stm{ 0 };
    };

    class CaptionManagementData {
    public:
        class Langage {
        public:
            uint8_t languageTag{ 0 };
            uint8_t dmf{ 0 };
            uint8_t dc{ 0 };
            std::string languageCode;
            uint8_t format{ 0 };
            uint8_t tcs{ 0 };
            uint8_t rollupMode{ 0 };
        };

        void setTmd(uint8_t tmd) {
            this->tmd = tmd;
        }

        void setOtm(uint64_t otm) {
            this->otm = otm;
        }

        bool pack(std::vector<uint8_t>& output) {
            output.push_back(tmd | 0b00111111); // TMD(2) | Reserved(6)
            if (tmd == 0b10) {
                output.push_back(static_cast<uint8_t>(otm >> 28));
                output.push_back(static_cast<uint8_t>(otm >> 20));
                output.push_back(static_cast<uint8_t>(otm >> 16));
                output.push_back(static_cast<uint8_t>(otm >> 8));
                output.push_back(static_cast<uint8_t>((otm & 0b1111) | 0b1111));
            }
            output.push_back(static_cast<uint8_t>(langages.size()));

            for (auto& langage : langages) {
                output.push_back(langage.languageTag << 5 | 1 << 4 | langage.dmf);
                if (langage.dmf == 0b1100 || langage.dmf == 0b1101 || langage.dmf == 0b1110) {
                    output.push_back(langage.dc);
                }
                output.push_back(langage.languageCode[0]);
                output.push_back(langage.languageCode[1]);
                output.push_back(langage.languageCode[2]);
                output.push_back(langage.format << 4 | langage.tcs << 2 | langage.rollupMode);
            }

            std::vector<uint8_t> packedDataUnit;

            for (auto& dataUnit : dataUnits) {
                if (!dataUnit.pack(packedDataUnit)) {
                    return false;
                }
            }

            size_t dataUnitSize = packedDataUnit.size();
            output.push_back(static_cast<uint8_t>(dataUnitSize >> 16));
            output.push_back(static_cast<uint8_t>(dataUnitSize >> 8));
            output.push_back(static_cast<uint8_t>(dataUnitSize));

            output.insert(output.end(), packedDataUnit.begin(), packedDataUnit.end());

            return true;
        }

        std::list<Langage> langages;
        std::vector<DataUnit> dataUnits;

    private:
        uint8_t tmd{ 0 };
        uint64_t otm{ 0 };
    };

    class DataGroup {
    public:
        void setGroupVersion(uint8_t version) {
            dataGroupVersion = version;
        }
        void setGroupLinkNumber(uint8_t linkNumber) {
            dataGroupLinkNumber = linkNumber;
        }
        void setLastGroupLinkNumber(uint8_t lastLinkNumber) {
            lastDataGroupLinkNumber = lastLinkNumber;
        }
        void setCaptionStatementIndex(uint8_t index) {
            captionStatementIndex = index;
        }

        void setGroupData(const std::variant<CaptionManagementData, CaptionStatementData>& data) {
            groupData = data;
        }

        bool pack(std::vector<uint8_t>& output) {
            uint8_t dataGroupId = 0;
            if (std::holds_alternative<CaptionManagementData>(groupData)) {
                dataGroupId = groupIndex == 0 ? 0 : 0x20;
            }
            else {
                dataGroupId = groupIndex == 0 ? 0 + captionStatementIndex + 1 : 0x20 + captionStatementIndex + 1;
            }

            output.push_back(dataGroupId << 2 | (dataGroupVersion & 0b00000011));
            output.push_back(dataGroupLinkNumber); // data_group_link_number(8)
            output.push_back(lastDataGroupLinkNumber); // last_data_group_link_number(8)

            std::vector<uint8_t> packed;
            std::visit([&packed](auto&& arg) {
                arg.pack(packed);
                }, groupData);

            size_t dataGroupSize = packed.size();
            output.push_back(static_cast<uint8_t>(dataGroupSize >> 8));
            output.push_back(static_cast<uint8_t>(dataGroupSize));
            output.insert(output.end(), packed.begin(), packed.end());

            uint16_t crc = crc16_calc(0, output);
            output.push_back(static_cast<uint8_t>(crc >> 8));
            output.push_back(static_cast<uint8_t>(crc));
            return true;
        }

    private:
        uint16_t crc16_calc(uint16_t crc, const std::vector<uint8_t>& buf) {
            constexpr uint16_t crctab[256] = {
                0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
                0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
                0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
                0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
                0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
                0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
                0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
                0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
                0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
                0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
                0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
                0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
                0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
                0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
                0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
                0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
            };
            for (auto byte : buf) {
                crc = (crc << 8) ^ crctab[((crc >> 8) ^ byte) & 0xFF];
            }
            return crc;
        }
        uint8_t dataGroupVersion{ 0 };
        uint8_t dataGroupLinkNumber{ 0 };
        uint8_t lastDataGroupLinkNumber{ 0 };

        // 0~7
        uint8_t captionStatementIndex{ 0 };

        // 0: Group A, 1: Group B
        uint8_t groupIndex{ 0 };

        std::variant<CaptionManagementData, CaptionStatementData> groupData;
    };

    class PESData {
    public:
        PESData(DataGroup& dataGroup) : dataGroup(dataGroup) {

        }
        enum class PESType {
            Synchronized,
            Asynchronous
        };

        void SetPESType(PESType type) {
            pesType = type;
        }

        PESType getPesType() const {
            return pesType;
        }

        std::vector<uint8_t> pesDataPacketHeader;


        bool pack(std::vector<uint8_t>& output) {
            std::array<uint8_t, 3> pesData;
            // data_identifier
            pesData[0] = pesType == PESType::Synchronized ? 0x80 : 0x81;
            // private_stream_id
            pesData[1] = 0xFF;
            // reserved_future_use(4) | PES_data_packet_header_length(4)
            pesData[2] = 0b11110000 | (pesDataPacketHeader.size() & 0b1111);

            if (pesDataPacketHeader.size() > 16) {
                throw std::runtime_error("pesDataPacketHeader size exceeds 16 bytes.");
                return false;
            }

            output.insert(output.end(), pesData.begin(), pesData.end());

            std::vector<uint8_t> packed;
            dataGroup.pack(packed);
            output.insert(output.end(), packed.begin(), packed.end());
            return true;
        }

    private:
        PESType pesType{ PESType::Synchronized };
        DataGroup& dataGroup;
    };

}

class B24SubtitleOutput {
public:
    B24SubtitleOutput() = default;
    B24SubtitleOutput(std::vector<uint8_t> pesData, std::optional<uint64_t> begin)
        : pesData(pesData), begin(begin) {}

    uint64_t calcPts(uint64_t programStartTime) const {
        if (!begin) {
            return 0;
        }
        return (programStartTime * 1000 + *begin) * 90;
    }

    std::vector<uint8_t> pesData;
    std::optional<uint64_t> begin;
};

class B24SubtitleConvertor {
public:
    static bool convert(const std::string& input, std::list<B24SubtitleOutput>& output);

};