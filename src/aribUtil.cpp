#include "aribUtil.h"
#include "tsARIBCharset.h"

struct B62Gaiji {
    const char8_t* replacement;
    union {
        char8_t utf8[4];
        uint32_t uint32;
    };
};

struct B24Symbol {
    union {
        char8_t utf8[4];
        uint32_t uint32;
    };
};

constexpr B62Gaiji b62GaijiTable[] = {
    {u8"[3D]",       {0xF0, 0x9F, 0x86, 0x9B}},
    {u8"[2nd Scr]",  {0xF0, 0x9F, 0x86, 0x9C}},
    {u8"[2K]",       {0xF0, 0x9F, 0x86, 0x9D}},
    {u8"[4K]",       {0xF0, 0x9F, 0x86, 0x9E}},
    {u8"[8K]",       {0xF0, 0x9F, 0x86, 0x9F}},
    {u8"[5.1]",      {0xF0, 0x9F, 0x86, 0xA0}},
    {u8"[7.1]",      {0xF0, 0x9F, 0x86, 0xA1}},
    {u8"[22.2]",     {0xF0, 0x9F, 0x86, 0xA2}},
    {u8"[60P]",      {0xF0, 0x9F, 0x86, 0xA3}},
    {u8"[120P]",     {0xF0, 0x9F, 0x86, 0xA4}},
    {u8"[ｄ]",        {0xF0, 0x9F, 0x86, 0xA5}},
    {u8"[HC]",       {0xF0, 0x9F, 0x86, 0xA6}},
    {u8"[HDR]",      {0xF0, 0x9F, 0x86, 0xA7}},
    {u8"[Hi-Res]",   {0xF0, 0x9F, 0x86, 0xA8}},
    {u8"[Lossless]", {0xF0, 0x9F, 0x86, 0xA9}},
    {u8"[SHV]",      {0xF0, 0x9F, 0x86, 0xAA}},
    {u8"[UHD]",      {0xF0, 0x9F, 0x86, 0xAB}},
    {u8"[VOD]",      {0xF0, 0x9F, 0x86, 0xAC}},
    {u8"[配]",       {0xF0, 0x9F, 0x88, 0xBB}},
    {u8"令和",       {0xE3, 0x8B, 0xBF, 0}}
};

namespace {

void replaceSequence(std::string& str, const char* sequence, size_t sequencelen, const char* replacement) {
    std::size_t pos = 0;

    while ((pos = str.find(sequence, pos)) != std::string::npos) {
        str.replace(pos, sequencelen, replacement);
        pos += 1;
    }
}
    
void convertB62Gaiji(std::string& str) {
    for (auto gaiji : b62GaijiTable) {
        if (gaiji.utf8[3] == 0) {
            replaceSequence(str, reinterpret_cast<const char*>(gaiji.utf8), 3, reinterpret_cast<const char*>(gaiji.replacement));
        }
        else {
            replaceSequence(str, reinterpret_cast<const char*>(gaiji.utf8), 4, reinterpret_cast<const char*>(gaiji.replacement));
        }
    }
}

}

const ts::ByteBlock aribEncode(const std::string& input) {
    std::string converted = input;
    convertB62Gaiji(converted);

    ts::UString text = ts::UString::FromUTF8(converted);
    auto aribBlock = ts::ARIBCharset2::B24.encoded(text);
    
    return aribBlock;
}

const ts::ByteBlock aribEncode(const char* input, size_t size) {
    return aribEncode(std::string{ input, size });
}
