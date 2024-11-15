#pragma once
#include <tsduck.h>

const ts::ByteBlock aribEncode(std::string input);

const ts::ByteBlock aribEncode(const char* input, size_t size);