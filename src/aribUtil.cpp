#include "aribUtil.h"
#include "tsARIBCharset.h"

struct Gaiji {
    const char8_t* find;
    const char8_t* replacement;
};

constexpr Gaiji GaijiTable[] = {
    // ARIB STD-B62
    {u8"\U0001F19B", u8"[3D]"},
    {u8"\U0001F19C", u8"[2nd Scr]"},
    {u8"\U0001F19D", u8"[2K]"},
    {u8"\U0001F19E", u8"[4K]"},
    {u8"\U0001F19F", u8"[8K]"},
    {u8"\U0001F1A0", u8"[5.1]"},
    {u8"\U0001F1A1", u8"[7.1]"},
    {u8"\U0001F1A2", u8"[22.2]"},
    {u8"\U0001F1A3", u8"[60P]"},
    {u8"\U0001F1A4", u8"[120P]"},
    {u8"\U0001F1A5", u8"[ｄ]"},
    {u8"\U0001F1A6", u8"[HC]"},
    {u8"\U0001F1A7", u8"[HDR]"},
    {u8"\U0001F1A8", u8"[Hi-Res]"},
    {u8"\U0001F1A9", u8"[Lossless]"},
    {u8"\U0001F1AA", u8"[SHV]"},
    {u8"\U0001F1AB", u8"[UHD]"},
    {u8"\U0001F1AC", u8"[VOD]"},
    {u8"\U0001F23B", u8"[配]"},
    {u8"\U000032FF", u8"令和"},

    // ARIB STD-B24 (Table 7-11 Addtional Kanji Characters)
    {u8"仿", u8"彷"},
    {u8"傜", u8"徭"},
    {u8"恵", u8"恵"},
    {u8"泠", u8"冷"},
    {u8"琢", u8"琢"},
    {u8"畵", u8"畫"},
    {u8"舘", u8"舘"},
    {u8"蟬", u8"蝉"},
    {u8"鷗", u8"鴎"},
    {u8"麴", u8"麹"},
    {u8"麵", u8"麺"},
    {u8"髙", u8"高"},
    {u8"塚", u8"塚"},
    {u8"﨑", u8"埼"},
    {u8"海", u8"海"},
    {u8"渚", u8"渚"},
    {u8"禮", u8"禮"},
    {u8"⾓", u8"角"},

    // https://github.com/ashtuchkin/iconv-lite/issues/145
    {u8"\U0000FF5E", u8"\U0000301C"}, //～ 〜
    {u8"\U00002225", u8"\U00002016"}, //∥ ‖
    {u8"\U0000FFE0", u8"\U000000A2"}, //￠ ¢
    {u8"\U0000FFE1", u8"\U000000A3"}, //￡ £
    {u8"\U0000FFE2", u8"\U000000AC"}, //￢ ¬
    {u8"\U0000FFE4", u8"\U000000A6"}, //￤ ¦
    
};

namespace {

void replaceSequence(std::string& str, const std::string& sequence, const char* replacement) {
    std::size_t pos = 0;

    while ((pos = str.find(sequence, pos)) != std::string::npos) {
        str.replace(pos, sequence.size(), replacement);
        pos += 1;
    }
}

void convertGaiji(std::string& str) {
    for (const auto& gaiji : GaijiTable) {
        replaceSequence(str, reinterpret_cast<const char*>(gaiji.find), reinterpret_cast<const char*>(gaiji.replacement));
    }
}

}

const ts::ByteBlock aribEncode(const std::string& input) {
    std::string converted = input;
    convertGaiji(converted);

    ts::UString text = ts::UString::FromUTF8(converted.data(), converted.size());
    return ts::ARIBCharset2::B24.encoded(text);
}


const ts::ByteBlock aribEncode(const char* input, size_t size) {
    return aribEncode(std::string{ input, size });
}
