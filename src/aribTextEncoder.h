#pragma once
#include "aribCharsetEncoder.h"
#include <cstddef>
#include <string>
#include <string_view>

namespace arib::text {

struct EncodeOptions {
    charset::EncodeMode mode = charset::EncodeMode::Text;
    bool replaceGaiji = true;
    bool normalizeHalfwidthKatakana = true;
    bool useCache = true;
};

std::string encode(std::string_view input, EncodeOptions options = {});
std::string encode(const char* input, std::size_t size, EncodeOptions options = {});

} // namespace arib::text
