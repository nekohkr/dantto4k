#pragma once

#include <cstdint>
#include <span>

namespace arib::charset {

enum class CharsetCode : uint8_t {
    None = 0,
    Kanji = 0x42,
    Alphanumeric = 0x4A,
    Hiragana = 0x30,
    Katakana = 0x31,
    MosaicA = 0x32,
    MosaicB = 0x33,
    MosaicC = 0x34,
    MosaicD = 0x35,
    PropAlphanumeric = 0x36,
    PropHiragana = 0x37,
    PropKatakana = 0x38,
    JISX0201Katakana = 0x49,
    JISKanjiPlane1 = 0x39,
    JISKanjiPlane2 = 0x3A,
    AdditionalSymbols = 0x3B,
    DRSC0 = 0x40,
    DRSC1 = 0x41,
    DRSC2 = 0x42,
    DRSC3 = 0x43,
    DRSC4 = 0x44,
    DRSC5 = 0x45,
    DRSC6 = 0x46,
    DRSC7 = 0x47,
    DRSC8 = 0x48,
    DRSC9 = 0x49,
    DRSC10 = 0x4A,
    DRSC11 = 0x4B,
    DRSC12 = 0x4C,
    DRSC13 = 0x4D,
    DRSC14 = 0x4E,
    DRSC15 = 0x4F,
    Macro = 0x70,
};

using Row = char32_t[94];

struct Charset {
    CharsetCode code;
    uint8_t proportionalCode;
    bool is2Byte;
    bool useSingleShift;
    bool useGR;
    uint8_t rowStart;
    uint8_t rowCount;
    const int8_t* rowIndex;
    const Row* rows;
};

struct CharsetPosition {
    CharsetCode charset;
    uint8_t row;
    uint8_t col;
};

struct CharAlias {
    char32_t codepoint;
    CharsetPosition target;
};

const Charset& alphanumeric();
const Charset& hiragana();
const Charset& katakana();
const Charset& jisX0201Katakana();
const Charset& jisKanjiPlane1();
const Charset& jisKanjiPlane2();
const Charset& additionalSymbols();
std::span<const CharAlias> additionalSymbolAliases();

const Charset* findCharset(CharsetCode code);
std::span<const Charset* const> lookupCharsets();

} // namespace arib::charset
