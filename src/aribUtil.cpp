#include "aribUtil.h"

const ts::ByteBlock aribEncode(std::string input) {
    ts::UString text = ts::UString::FromUTF8(input);
    return ts::ARIBCharset::B24.encoded(std::move(text));
}

const ts::ByteBlock aribEncode(const char* input, size_t size) {
    ts::UString text = ts::UString::FromUTF8(input, size);
    return ts::ARIBCharset::B24.encoded(std::move(text));
}
