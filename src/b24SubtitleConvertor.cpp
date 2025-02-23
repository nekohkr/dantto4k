#include "b24SubtitleConvertor.h"
#include <vector>
#include "aribUtil.h"
#include "b24Color.h"
#include "b24ControlSet.h"

namespace {

    std::vector<std::string> splitEncodedByNull(const std::vector<uint8_t>& data) {
        std::vector<std::string> tokens;
        std::string current;
        for (auto byte : data) {
            if (byte == 0) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            else {
                current.push_back(static_cast<char>(byte));
            }
        }
        if (!current.empty()) {
            tokens.push_back(current);
        }
        return tokens;
    }

    void appendNumber(std::vector<uint8_t>& output, int n) {
        if (n == 0) {
            output.push_back(0x30);
            return;
        }
        std::vector<uint8_t> temp;
        while (n > 0) {
            temp.push_back(static_cast<uint8_t>((n % 10) + 0x30));
            n /= 10;
        }
        std::reverse(temp.begin(), temp.end());
        output.insert(output.end(), temp.begin(), temp.end());
    }

}


bool B24SubtiteConvertor::convert(const std::vector<uint8_t>& input, std::list<B24SubtiteOutput>& output) {
    TTML ttml = TTMLPaser::parse(input);

    uint8_t lastTextColorPalette = 0;
    uint8_t lastTextColorIndex = 7;
    uint8_t lastBackgroundColorPalette = 0;
    uint8_t lastBackgroundColorIndex = 8;

    for (const auto& div : ttml.divTags) {
        B24::CaptionStatementData captionStatementData;
        std::vector<uint8_t> unitDataByte;

        std::string text;
        for (const auto& p : div.pTags) {
            for (const auto& span : p.spanTags) {
                std::u8string utf((char8_t*)(span.text.c_str()));
                text.append(span.text);
                text.push_back('\0');
            }
        }

        auto encoded = aribEncode(text);
        auto encodedSplit = splitEncodedByNull(encoded);
        int encodedSplitIndex = 0;

        // clear
        unitDataByte.push_back(B24ControlSet::CS);

        for (const auto& p : div.pTags) {
            if (p.region.extent.has_value()) {
                unitDataByte.push_back(B24ControlSet::CSI);
                appendNumber(unitDataByte, static_cast<uint32_t>(p.region.extent->first.getValue<TTMLCssValueLength>().value * 960 / 3840));
                unitDataByte.push_back(0x3B);
                appendNumber(unitDataByte, static_cast<uint32_t>(p.region.extent->second.getValue<TTMLCssValueLength>().value * 540 / 2160));
                unitDataByte.push_back(B24ControlSet::SP);
                unitDataByte.push_back(B24ControlSet::SDF);
            }

            if (p.region.origin.has_value()) {
                unitDataByte.push_back(B24ControlSet::CSI);
                appendNumber(unitDataByte, static_cast<uint32_t>(p.region.origin->first.getValue<TTMLCssValueLength>().value * 960 / 3840));
                unitDataByte.push_back(0x3B);
                appendNumber(unitDataByte, static_cast<uint32_t>(p.region.origin->second.getValue<TTMLCssValueLength>().value * 540 / 2160));
                unitDataByte.push_back(B24ControlSet::SP);
                unitDataByte.push_back(B24ControlSet::SDP);
            }
            
            // set active position to 0 x 0
            unitDataByte.push_back(B24ControlSet::APS);
            unitDataByte.push_back(0x40);
            unitDataByte.push_back(0x40);

            TTMLStyle oldStyle;
            for (const auto& span : p.spanTags) {
                if (span.style.backgroundColor.has_value()) {
                    TTMLCssValueColor color = span.style.backgroundColor->getValue<TTMLCssValueColor>();
                    auto closetColor = findClosestColor(ColorRGBA{ color.r, color.g, color.b, color.a });

                    if (lastBackgroundColorPalette != closetColor.first || lastBackgroundColorIndex != closetColor.second) {
                        unitDataByte.push_back(B24ControlSet::COL);
                        unitDataByte.push_back(0x20);
                        unitDataByte.push_back(0x40 | closetColor.first);
                        unitDataByte.push_back(B24ControlSet::COL);
                        unitDataByte.push_back(0x50 | closetColor.second);

                        lastBackgroundColorPalette = closetColor.first;
                        lastBackgroundColorIndex = closetColor.second;
                    }
                }
                if (span.style.color.has_value()) {
                    TTMLCssValueColor color = span.style.color->getValue<TTMLCssValueColor>();
                    auto closetColor = findClosestColor(ColorRGBA{ color.r, color.g, color.b, color.a });

                    if (lastTextColorPalette != closetColor.first || lastTextColorIndex != closetColor.second) {
                        if (closetColor.first == 0 && closetColor.second >= 0 && closetColor.second <= 7) {
                            unitDataByte.push_back(B24ControlSet::COL);
                            unitDataByte.push_back(0x20);
                            unitDataByte.push_back(0x40 | 0);
                            unitDataByte.push_back(B24ControlSet::BKF + closetColor.second);
                        }
                        else {
                            unitDataByte.push_back(B24ControlSet::COL);
                            unitDataByte.push_back(0x20);
                            unitDataByte.push_back(0x40 | closetColor.first);
                            unitDataByte.push_back(B24ControlSet::COL);
                            unitDataByte.push_back(0x40 | closetColor.second);
                        }

                        lastTextColorPalette = closetColor.first;
                        lastTextColorIndex = closetColor.second;
                    }
                }
                if (span.style.fontSize.has_value()) {
                    TTMLCssValueLength first = span.style.fontSize->first.getValue<TTMLCssValueLength>();
                    TTMLCssValueLength second = span.style.fontSize->second.getValue<TTMLCssValueLength>();

                    if (first.value == 144 && second.value == 144) {
                        unitDataByte.push_back(B24ControlSet::NSZ);
                    }
                    else if (first.value == 72 && second.value == 144) {
                        unitDataByte.push_back(B24ControlSet::MSZ);
                    }
                    else if (first.value == 72 && second.value == 72) {
                        unitDataByte.push_back(B24ControlSet::SSZ);
                    }
                }

                unitDataByte.insert(unitDataByte.end(), encodedSplit[encodedSplitIndex].begin(), encodedSplit[encodedSplitIndex].end());
                encodedSplitIndex++;
            }
        }

        captionStatementData.dataUnits.push_back({ unitDataByte });

        B24::DataGroup dataGroup;
        dataGroup.setGroupData(captionStatementData);

        std::vector<uint8_t> packedPesData;
        B24::PESData pesData(dataGroup);
        pesData.SetPESType(B24::PESData::PESType::Synchronized);
        pesData.pack(packedPesData);

        output.push_back({ packedPesData , div.begin, div.end });
    }

    return true;  
}