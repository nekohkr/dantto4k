#include "b24SubtitleConverter.h"
#include "aribTextEncoder.h"
#include "b24Color.h"
#include "b24ControlSet.h"
#include "ttml/parser.h"
#include "ttml/resolver.h"
#include "ttml/b24_converter.h"
#include <cmath>
#include <vector>

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

void appendCharacterComposition(std::vector<uint8_t>& output, uint32_t width, uint32_t height) {
    output.push_back(B24ControlSet::CSI);
    appendNumber(output, width);
    output.push_back(0x3B);
    appendNumber(output, height);
    output.push_back(B24ControlSet::SP);
    output.push_back(B24ControlSet::SSM);
}

void appendSpacing(std::vector<uint8_t>& output, uint32_t spacing, uint8_t control) {
    output.push_back(B24ControlSet::CSI);
    appendNumber(output, spacing);
    output.push_back(B24ControlSet::SP);
    output.push_back(control);
}

void appendTwoDigits(std::vector<uint8_t>& output, uint8_t value) {
    output.push_back(static_cast<uint8_t>(0x30 + value / 10));
    output.push_back(static_cast<uint8_t>(0x30 + value % 10));
}

void appendOrnament(std::vector<uint8_t>& output, uint8_t palette, uint8_t index) {
    output.push_back(B24ControlSet::CSI);
    output.push_back(0x31);
    output.push_back(0x3B);
    appendTwoDigits(output, palette);
    appendTwoDigits(output, index);
    output.push_back(B24ControlSet::SP);
    output.push_back(B24ControlSet::ORN);
}

void appendOrnamentOff(std::vector<uint8_t>& output) {
    output.push_back(B24ControlSet::CSI);
    output.push_back(0x30);
    output.push_back(B24ControlSet::SP);
    output.push_back(B24ControlSet::ORN);
}

}

bool B24SubtitleConverter::convert(const std::string& input, std::list<B24SubtitleOutput>& output, B24::PESData::PESType pesType) {
    std::string_view ttml_input = input;

    auto parse_result = arib::ttml::parse(ttml_input);
    if (parse_result.error) {
        std::cerr << "[TTML] " << parse_result.error->message << std::endl;
        return false;
    }
    auto resolve_result = arib::ttml::resolve(*parse_result.document);
    if (resolve_result.error) {
        std::cerr << "[TTML] " << resolve_result.error->message << std::endl;
        return false;
    }
    auto convert_result = arib::ttml::convert_to_b24(*resolve_result.document, 
        pesType == B24::PESData::PESType::Synchronized
            ? arib::ttml::PESType::Sync
            : arib::ttml::PESType::Async);
    if (convert_result.error) {
        std::cerr << "[TTML] " << convert_result.error->message << std::endl;
        return false;
    }

    for (auto& converted : convert_result.outputs) {
        B24::CaptionStatementData captionStatementData;
        captionStatementData.dataUnits.push_back({ {B24ControlSet::CS} });
        if (converted.data.size()) {
            captionStatementData.dataUnits.push_back(std::move(converted.data));
        }

        B24::DataGroup dataGroup;
        dataGroup.setGroupData(captionStatementData);

        std::vector<uint8_t> packedPesData;
        B24::PESData pesData(dataGroup);
        pesData.SetPESType(pesType);
        pesData.pack(packedPesData);

        output.push_back({
            packedPesData,
            converted.begin
                ? std::optional<uint64_t>{static_cast<uint64_t>(converted.begin->count())}
                : std::nullopt,
        });
    }

    return true;
}
