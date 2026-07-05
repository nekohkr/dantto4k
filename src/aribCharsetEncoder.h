#pragma once

#include <string>
#include <string_view>

namespace arib::charset {

enum class EncodeMode {
    Text,
    Caption,
};

std::string encode(std::string_view input, EncodeMode mode = EncodeMode::Text);
std::string encode(std::u8string_view input, EncodeMode mode = EncodeMode::Text);

} // namespace arib::charset
