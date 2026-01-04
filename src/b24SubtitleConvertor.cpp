#include "b24SubtitleConvertor.h"
#include <vector>
#include "aribUtil.h"
#include "b24Color.h"
#include "b24ControlSet.h"

namespace {

    std::vector<std::string> splitByNull(const std::string& data) {
        std::vector<std::string> tokens;
        std::string current;
        for (auto byte : data) {
            if (byte == 0) {
                tokens.push_back(current);
                current.clear();
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

bool B24SubtitleConvertor::convert(const std::string& input, std::list<B24SubtitleOutput>& output) {
    const TTML ttml = TTMLPaser::parse(input);

    uint8_t lastTextColorPalette = 0;
    uint8_t lastTextColorIndex = 7;
    uint8_t lastBackgroundColorPalette = 0;
    uint8_t lastBackgroundColorIndex = 8;
    uint8_t characterSize = B24ControlSet::NSZ;
    double fontHeight = 0;

    for (const auto& div : ttml.divTags) {
        B24::CaptionStatementData captionStatementData;

        std::string text;
        for (const auto& p : div.pTags) {
            for (const auto& span : p.spanTags) {
                text.append(span.text);
                text.push_back('\0');
            }
        }

        auto encoded = aribEncode(text, true);
        auto encodedSplit = splitByNull(encoded);
        int encodedSplitIndex = 0;

        std::vector<uint8_t> unitDataByte;
        unitDataByte.push_back(B24ControlSet::CS);

        // Set writing format 960 x 540
        unitDataByte.push_back(B24ControlSet::CSI);
        unitDataByte.push_back(0x37);
        unitDataByte.push_back(B24ControlSet::SP);
        unitDataByte.push_back(B24ControlSet::SWF);

        for (const auto& p : div.pTags) {
            if (p.spanTags.empty()) {
                continue;
            }

            // Set display format
            if (p.region.extent.has_value()) {
                unitDataByte.push_back(B24ControlSet::CSI);
                appendNumber(unitDataByte, static_cast<uint32_t>(p.region.extent->first.getValue<TTMLCssValueLength>().value * 960 / 3840));
                unitDataByte.push_back(0x3B);
                appendNumber(unitDataByte, static_cast<uint32_t>(p.region.extent->second.getValue<TTMLCssValueLength>().value * 540 / 2160));
                unitDataByte.push_back(B24ControlSet::SP);
                unitDataByte.push_back(B24ControlSet::SDF);
            }

            // Set character size
            const auto firstSpan = p.spanTags.begin();
            if (firstSpan != p.spanTags.end() && firstSpan->style.fontSize) {
                TTMLCssValueLength first = firstSpan->style.fontSize->first.getValue<TTMLCssValueLength>();
                TTMLCssValueLength second = firstSpan->style.fontSize->second.getValue<TTMLCssValueLength>();

                if (first.value == 144 && second.value == 144) {
                    if (characterSize != B24ControlSet::NSZ) {
                        unitDataByte.push_back(B24ControlSet::NSZ);
                        characterSize = B24ControlSet::NSZ;
                    }
                    fontHeight = 240;
                }
                else if (first.value == 72 && second.value == 144) {
                    if (characterSize != B24ControlSet::MSZ) {
                        unitDataByte.push_back(B24ControlSet::MSZ);
                        characterSize = B24ControlSet::MSZ;
                    }
                    fontHeight = 240;
                }
                else if (first.value == 72 && second.value == 72) {
                    if (characterSize != B24ControlSet::SSZ) {
                        unitDataByte.push_back(B24ControlSet::SSZ);
                        characterSize = B24ControlSet::SSZ;
                    }
                    fontHeight = 120;
                }
            }

            // Set position
            if (p.region.origin.has_value()) {
                double offsetY = 0;
                if (p.spanTags.begin()->style.lineHeight.has_value() &&
                    p.spanTags.begin()->style.fontSize) {
                    double lineHeight = p.spanTags.begin()->style.lineHeight->getValue<TTMLCssValueLength>().value;
                    if (p.region.extent.has_value()) {
                        offsetY = (lineHeight - fontHeight) / 2;
                    }
                }

                unitDataByte.push_back(B24ControlSet::CSI);
                appendNumber(unitDataByte, static_cast<uint32_t>(p.region.origin->first.getValue<TTMLCssValueLength>().value * 960 / 3840));
                unitDataByte.push_back(0x3B);
                appendNumber(unitDataByte, static_cast<uint32_t>((p.region.origin->second.getValue<TTMLCssValueLength>().value + offsetY) * 540 / 2160));
                unitDataByte.push_back(B24ControlSet::SP);
                unitDataByte.push_back(B24ControlSet::SDP);
            }

            // Set active position to 0 x 0
            unitDataByte.push_back(B24ControlSet::APS);
            unitDataByte.push_back(0x40);
            unitDataByte.push_back(0x40);

            TTMLStyle oldStyle;
            for (const auto& span : p.spanTags) {
                // Set character size
                if (span.style.fontSize) {
                    TTMLCssValueLength first = span.style.fontSize->first.getValue<TTMLCssValueLength>();
                    TTMLCssValueLength second = span.style.fontSize->second.getValue<TTMLCssValueLength>();

                    if (first.value == 144 && second.value == 144) {
                        if (characterSize != B24ControlSet::NSZ) {
                            unitDataByte.push_back(B24ControlSet::NSZ);
                            characterSize = B24ControlSet::NSZ;
                        }
                    }
                    else if (first.value == 72 && second.value == 144) {
                        if (characterSize != B24ControlSet::MSZ) {
                            unitDataByte.push_back(B24ControlSet::MSZ);
                            characterSize = B24ControlSet::MSZ;
                        }
                    }
                    else if (first.value == 72 && second.value == 72) {
                        if (characterSize != B24ControlSet::SSZ) {
                            unitDataByte.push_back(B24ControlSet::SSZ);
                            characterSize = B24ControlSet::SSZ;
                        }
                    }
                }

                // Set background color
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

                // Set text color
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


                unitDataByte.insert(unitDataByte.end(), encodedSplit[encodedSplitIndex].begin(), encodedSplit[encodedSplitIndex].end());
                encodedSplitIndex++;
            }
        }

        captionStatementData.dataUnits.push_back({ unitDataByte });

        if (div.begin && div.end) {
            std::vector<uint8_t> unitDataByte;

            uint64_t duration = (*div.end - *div.begin) / 100;
            while (duration > 0) {
                uint8_t value = static_cast<uint8_t>(std::min(duration, static_cast<uint64_t>(0x3F)));
                unitDataByte.push_back(B24ControlSet::TIME);
                unitDataByte.push_back(0x20);
                unitDataByte.push_back(0x40 | value);
                unitDataByte.push_back(0x0C);
                duration -= value;
            }

            captionStatementData.dataUnits.push_back({ unitDataByte });
        }

        B24::DataGroup dataGroup;
        dataGroup.setGroupData(captionStatementData);

        std::vector<uint8_t> packedPesData;
        B24::PESData pesData(dataGroup);
        pesData.SetPESType(B24::PESData::PESType::Synchronized);
        pesData.pack(packedPesData);

        output.push_back({ packedPesData , div.begin });
    }

    if (output.size() == 0) {
        B24::CaptionStatementData captionStatementData;
        {
            // clear
            std::vector<uint8_t> unitDataByte;
            unitDataByte.push_back(B24ControlSet::CS);
            captionStatementData.dataUnits.push_back({ unitDataByte });
        }
        B24::DataGroup dataGroup;
        dataGroup.setGroupData(captionStatementData);

        std::vector<uint8_t> packedPesData;
        B24::PESData pesData(dataGroup);
        pesData.SetPESType(B24::PESData::PESType::Synchronized);
        pesData.pack(packedPesData);

        output.push_back({ packedPesData , 0 });
    }

    return true;
}