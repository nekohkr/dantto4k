#include "b24SubtitleConverter.h"
#include "aribTextEncoder.h"
#include "b24Color.h"
#include "b24ControlSet.h"
#include "ttml/parser.h"
#include "ttml/resolver.h"
#include "ttml/b24_converter.h"
#include <cmath>
#include <vector>

bool B24SubtitleConverter::convert(const std::string& input, std::list<B24SubtitleOutput>& output, B24::PESData::PESType pesType, MmtTlv::SubtitleResolution resolution) {
    const auto sync_mode = pesType == B24::PESData::PESType::Synchronized
        ? arib::ttml::SyncMode::Sync
        : arib::ttml::SyncMode::Async;
    auto parse_result = arib::ttml::parse(input, sync_mode);
    if (parse_result.error) {
#ifdef _DEBUG
        std::cerr << "[TTML] " << parse_result.error->message << std::endl;
#endif
        return false;
    }
    auto resolve_result = arib::ttml::resolve(*parse_result.document, sync_mode);
    if (resolve_result.error) {
#ifdef _DEBUG
        std::cerr << "[TTML] " << resolve_result.error->message << std::endl;
#endif
        return false;
    }
    auto convert_result = arib::ttml::convert_to_b24(*resolve_result.document, sync_mode, resolution);
    if (convert_result.error) {
#ifdef _DEBUG
        std::cerr << "[TTML] " << convert_result.error->message << std::endl;
#endif
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
