#pragma once
#include "aribCharsetEncoder.h"
#include <cstddef>
#include <string>
#include <string_view>

namespace arib::text {

std::string encode(std::string_view input, charset::EncodeMode mode = charset::EncodeMode::Text);
std::string encode(const char* input, std::size_t size, charset::EncodeMode mode = charset::EncodeMode::Text);

} // namespace arib::text
