#pragma once
#include <string>
#include <string_view>

const std::string aribEncode(std::string_view input, bool isCaption = false);
const std::string aribEncode(const char* input, size_t size, bool isCaption = false);
