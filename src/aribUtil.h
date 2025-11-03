#pragma once
#include <string>

const std::string aribEncode(const std::string& input, bool isCaption = false);
const std::string aribEncode(const char* input, size_t size, bool isCaption = false);
