#pragma once
#include <cstdint>
#include <string>
#include <codecvt>
#include <map>
#include <array>
#include <optional>
#include <unordered_map>
#include <set>
#include <vector>

class AribEncoder {
public:
    static std::string encode(const std::string& input, bool isCaption = false) {
        AribEncoder AribEncoder(isCaption);
        return AribEncoder.encodeImpl(std::u8string(input.begin(), input.end()));
    }
    static std::string encode(const std::u8string& input, bool isCaption = false) {
        AribEncoder AribEncoder(isCaption);
        return AribEncoder.encodeImpl(input);
    }

private:
    static constexpr uint8_t APR = 0x0D;
    static constexpr uint8_t LS1 = 0x0E;
    static constexpr uint8_t LS0 = 0x0F;
    static constexpr uint8_t SS2 = 0x19;
    static constexpr uint8_t ESC = 0x1B;
    static constexpr uint8_t SS3 = 0x1D;
    static constexpr uint8_t SP = 0x20;
    static constexpr uint8_t SSZ = 0x88;
    static constexpr uint8_t MSZ = 0x89;
    static constexpr uint8_t NSZ = 0x8A;

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

    static inline const char32_t fullwidthChars[] = U"！”＃＄％＆’（）＊＋，−－～．／０１２３４５６７８９：；＜＝＞？＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ［￥］＾＿‘ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ｛｜｝￣";
    static inline const char32_t halfwidthChars[] = U"!\"#$%&'()*+,--~./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

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
    
    bool isCaption;
    std::array<CharsetCode, 4> graphic;
    bool isGR{false};
    uint8_t gl{0};
    uint8_t gr{2};
    uint8_t singleShift = 0;
    CharsetCode prevCharsetCode{ CharsetCode::None };
    uint8_t characterSize = NSZ;
    std::string output;
    std::u32string u32;

    static const Charset alphanumeric;
    static const Row alphanumericRow[];

    static const Charset hiragana;
    static const Row hiraganaRow[];

    static const Charset katakana;
    static const Row katakanaRow[];

    static const Charset jisX0201Katakana;
    static const Row jisX0201KatakanaRow[];

    static const Charset jisKanjiPlane1;
    static const Row jisKanjiPlane1Row[];

    static const Charset jisKanjiPlane2;
    static const int8_t jisKanjiPlane2RowIndex[];
    static const Row jisKanjiPlane2Row[];

    static const Charset additionalSymbols;
    static const uint8_t additionalSymbolsRowIndex[];
    static const Row additionalSymbolsRow[];

    static inline const std::map<CharsetCode, const Charset*> mapCharset = {
        {alphanumeric.code, &alphanumeric},
        {hiragana.code, &hiragana},
        {katakana.code, &katakana},
        {jisX0201Katakana.code, &jisX0201Katakana},
        {jisKanjiPlane1.code, &jisKanjiPlane1},
        {jisKanjiPlane2.code, &jisKanjiPlane2},
        {additionalSymbols.code, &additionalSymbols},
    };

private:
    AribEncoder(bool isCaption)
        : isCaption(isCaption) {
        if (isCaption) {
            graphic = { CharsetCode::JISKanjiPlane1, CharsetCode::Alphanumeric, CharsetCode::Hiragana, CharsetCode::Macro };
        }
        else {
            graphic = { CharsetCode::JISKanjiPlane1, CharsetCode::Alphanumeric, CharsetCode::Hiragana, CharsetCode::Katakana };
        }
    }

    bool isFullwidth(char32_t ch) {
        for (const char32_t* p = fullwidthChars; *p != 0; ++p) {
            if (*p == ch) {
                return true;
            }
        }
        return false;
    }

    char32_t toHalfwidth(char32_t ch) {
        for (size_t i = 0; fullwidthChars[i] != 0; ++i) {
            if (fullwidthChars[i] == ch) {
                return halfwidthChars[i];
            }
        }
        return ch;
    }

    std::string encodeImpl(std::u8string input) {
        u32 = toU32(input);
        size_t pos = 0;
        for (char32_t c : u32) {
            bool fullwidth = isFullwidth(c);
            if (fullwidth) {
                c = toHalfwidth(c);
            }

            if (c == '\0') {
                output.push_back(0);
                ++pos;
                continue;
            }

            if (c == '\n') {
                output.push_back(APR);
                ++pos;
                continue;
            }

            if (c == ' ') {
                if (!isCaption) {
                    if (fullwidth) {
                        setCharacterSize(NSZ);
                    }
                    else {
                        setCharacterSize(MSZ);
                    }
                }
                output.push_back(SP);
                ++pos;
                continue;
            }

            CharsetCode glCode = graphic[gl];
            CharsetCode grCode = graphic[gr];
            auto find = findChar(c, glCode, grCode);

            if (!find) {
                ++pos;
                continue;
            }

            // charset is not in GL or GR
            if (u32.size() > pos + 1 &&
                find->charset->code != glCode && find->charset->code != grCode) {

                // If the character is not Hiragana or Katakana
                if (!(c >= 0x3041 && c <= 0x3093) && !(c >= 0x30A1 && c <= 0x30F6)) {
                    // check if the current and next characters belong to the same charset
                    auto find2 = findCharsetBy2Char(c, u32[pos + 1], glCode, grCode);
                    if (find2) {
                        find = find2;
                    }
                }
            }

            selectCharset(find->charset, pos);

            if (!isCaption) {
                if (find->charset->code == CharsetCode::Alphanumeric) {
                    if (fullwidth) {
                        setCharacterSize(NSZ);
                    }
                    else {
                        setCharacterSize(MSZ);
                    }
                }
                else {
                    setCharacterSize(NSZ);
                }
            }

            const uint8_t mask = isGR ? 0x80 : 0x00;
            if (find->charset->is2Byte) {
                output.push_back((find->row + 0x21) | mask);
            }
            output.push_back((find->col + 0x21) | mask);
            ++pos;
        }
        return output;
    }

    void setCharacterSize(uint8_t size) {
        if (characterSize != size) {
            output.push_back(size);
            characterSize = size;
        }
    }

    void selectCharset(const Charset* charset, size_t pos) {
        uint8_t graphicIndex = findGraphicByCharsetCode(charset->code);

        // Check single shift
        if (charset->code == CharsetCode::Hiragana || charset->code == CharsetCode::Katakana) {
            if (prevCharsetCode != charset->code) {
                if (getCharsetRunLength(charset->code, pos) <= 2) {
                    if (graphicIndex == 2 || graphicIndex == 3) {
                        singleShift = graphicIndex;
                    }
                    else {
                        singleShift = 0;
                    }
                }
                else {
                    singleShift = 0;
                }
            }
        }
        else {
            singleShift = 0;
        }

        if (gl == graphicIndex) {
            isGR = false;
            prevCharsetCode = charset->code;
            return;
        }
        if (gr == graphicIndex) {
            isGR = true;
            prevCharsetCode = charset->code;
            return;
        }

        if (singleShift) {
            output.push_back(singleShift == 2 ? SS2 : SS3);
            isGR = false;
            prevCharsetCode = charset->code;
            return;
        }

        if (graphicIndex != 0xFF) {
            // Check the next charset to decide whether to use GL or GR
            CharsetCode glCode = graphic[gl];
            CharsetCode grCode = graphic[gr];

            bool useGR = charset->useGR;
            if (charset->code == CharsetCode::Hiragana || charset->code == CharsetCode::Katakana) {
                const Charset* nextCharset = getNextCharset(charset->code, pos);
                if (nextCharset) {
                    if (nextCharset->code == glCode) {
                        useGR = true;
                    }
                    if (nextCharset->code == grCode) {
                        useGR = false;
                    }
                }
            }

            if (useGR) {
                setGR(graphicIndex);
            }
            else {
                setGL(graphicIndex);
            }
        }
        else {
            // charset is not in G0–G3
            setGraphicToCharset(0, charset->code);
            setGL(0);
        }

        prevCharsetCode = charset->code;
    }

    void setGraphicToCharset(uint8_t graphicIndex, CharsetCode charsetCode) {
        if (graphicIndex < 0 || graphicIndex > 3) {
            return;
        }

        auto charset = mapCharset.at(charsetCode);

        output.push_back(ESC);
        if (charset->is2Byte == false) {
            output.push_back(0x28 + graphicIndex);
            output.push_back(static_cast<uint8_t>(charsetCode));
        }
        else {
            output.push_back(0x24);
            if (graphicIndex >= 1 && graphicIndex <= 3) {
                output.push_back(0x29 + graphicIndex);
            }
            output.push_back(static_cast<uint8_t>(charsetCode));
        }

        graphic[graphicIndex] = static_cast<CharsetCode>(charsetCode);
    }

    void setGL(uint8_t graphicIndex) {
        isGR = false;
        if (gl == graphicIndex) {
            return;
        }
        gl = graphicIndex;

        if (graphicIndex == 0) {
            output.push_back(LS0);
        }
        else if (graphicIndex == 1) {
            output.push_back(LS1);
        }
        else if (graphicIndex == 2) {
            output.push_back(ESC);
            output.push_back(0x6E);
        }
        else if (graphicIndex == 3) {
            output.push_back(ESC);
            output.push_back(0x6F);
        }
    }

    void setGR(uint8_t graphicIndex) {
        isGR = true;
        if (gr == graphicIndex) {
            return;
        }
        gr = graphicIndex;

        if (graphicIndex == 0) {
        }
        else if (graphicIndex == 1) {
            output.push_back(ESC);
            output.push_back(0x7E);
        }
        else if (graphicIndex == 2) {
            output.push_back(ESC);
            output.push_back(0x7D);
        }
        else if (graphicIndex == 3) {
            output.push_back(ESC);
            output.push_back(0x7C);
        }
    }

    uint8_t findGraphicByCharsetCode(CharsetCode charsetCode) {
        for (uint8_t i = 0; i < 4; i++) {
            if (graphic[i] == charsetCode) {
                return i;
            }
        }

        return -1;
    }

    size_t getCharsetRunLength(CharsetCode charsetCode, size_t pos) {
        size_t count = 0;
        for (size_t i = pos; i < u32.size(); i++) {
            auto result = findChar(u32[i], charsetCode, CharsetCode::None);
            if (!result) {
                return count;
            }

            if (charsetCode == result->charset->code) {
                count++;
            }
            else {
                return count;
            }
        }
        return count;
    };

    const Charset* getNextCharset(CharsetCode currentCharsetCode, size_t pos) {
        for (size_t i = pos; i < u32.size(); i++) {
            auto result = findChar(u32[i], currentCharsetCode, CharsetCode::None);
            if (!result) {
                return nullptr;
            }

            if (currentCharsetCode != result->charset->code) {
                return result->charset;
            }
        }
        return nullptr;
    };

    std::u32string toU32(const std::u8string& input) {
        std::u32string result;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(input.data());
        size_t len = input.size();

        for (size_t i = 0; i < len;) {
            uint32_t codepoint = 0;
            if (p[i] < 0x80) {
                codepoint = p[i++];
            }
            else if ((p[i] & 0xE0) == 0xC0 && i + 1 < len) {
                codepoint = ((p[i] & 0x1F) << 6) | (p[i + 1] & 0x3F);
                i += 2;
            }
            else if ((p[i] & 0xF0) == 0xE0 && i + 2 < len) {
                codepoint = ((p[i] & 0x0F) << 12) | ((p[i + 1] & 0x3F) << 6) | (p[i + 2] & 0x3F);
                i += 3;
            }
            else if ((p[i] & 0xF8) == 0xF0 && i + 3 < len) {
                codepoint = ((p[i] & 0x07) << 18) | ((p[i + 1] & 0x3F) << 12)
                    | ((p[i + 2] & 0x3F) << 6) | (p[i + 3] & 0x3F);
                i += 4;
            }
            else {
                ++i;
                continue;
            }
            result.push_back(static_cast<char32_t>(codepoint));
        }

        return result;
    }

    struct findResult {
        const Charset* charset;
        uint8_t row;
        uint8_t col;
    };

    std::optional<findResult> findCharInCharset(char32_t c, const Charset* charset) {
        for (uint8_t row = 0; row < charset->rowCount; row++) {
            if (charset->rowIndex) {
                if (charset->rowIndex[row] == -1) {
                    continue;
                }
            }

            for (uint8_t col = 0; col < 94; col++) {
                const uint8_t actualRow = charset->rowIndex ? charset->rowIndex[row] : row;
                if (charset->rows[actualRow][col] == c) {
                    findResult result = {
                        charset,
                        static_cast<uint8_t>(row + charset->rowStart),
                        col
                    };
                    return result;
                }
            }
        }

        return std::nullopt;
    }

    std::optional<findResult> findChar(char32_t c, CharsetCode candidate1, CharsetCode candidate2) {
        std::vector<const Charset*> charsets;
        std::set<const Charset*> seen;
        if (candidate1 != CharsetCode::None &&
            candidate1 != CharsetCode::JISKanjiPlane1 &&
            candidate1 != CharsetCode::JISKanjiPlane2) {
            if (seen.insert(mapCharset.at(candidate1)).second) {
                charsets.push_back(mapCharset.at(candidate1));
            }
        }
        if (candidate2 != CharsetCode::None &&
            candidate2 != CharsetCode::JISKanjiPlane1 &&
            candidate2 != CharsetCode::JISKanjiPlane2) {
            if (seen.insert(mapCharset.at(candidate2)).second) {
                charsets.push_back(mapCharset.at(candidate2));
            }
        }

        for (const auto* charset : { &alphanumeric, &hiragana, &katakana, &jisX0201Katakana, &jisKanjiPlane1,
                                    &jisKanjiPlane2, &additionalSymbols }) {
            if (seen.insert(charset).second) {
                charsets.push_back(charset);
            }
        }

        for (const auto* charset : charsets) {
            auto result = findCharInCharset(c, charset);
            if (result) {
                return result;
            }
        }
        return std::nullopt;
    }

    std::optional<findResult> findCharsetBy2Char(char32_t c1, char32_t c2, CharsetCode candidate1, CharsetCode candidate2) {
        std::vector<const Charset*> charsets;
        std::set<const Charset*> seen;

        if (candidate1 != CharsetCode::None &&
            candidate1 != CharsetCode::JISKanjiPlane1 &&
            candidate1 != CharsetCode::JISKanjiPlane2) {
            if (seen.insert(mapCharset.at(candidate1)).second) {
                charsets.push_back(mapCharset.at(candidate1));
            }
        }
        if (candidate2 != CharsetCode::None &&
            candidate2 != CharsetCode::JISKanjiPlane1 &&
            candidate2 != CharsetCode::JISKanjiPlane2) {
            if (seen.insert(mapCharset.at(candidate2)).second) {
                charsets.push_back(mapCharset.at(candidate2));
            }
        }

        for (const auto* charset : { &alphanumeric, &hiragana, &katakana, &jisX0201Katakana, &jisKanjiPlane1,
                                    &jisKanjiPlane2, &additionalSymbols }) {
            if (seen.insert(charset).second) {
                charsets.push_back(charset);
            }
        }

        for (const auto* charset : charsets) {
            auto result1 = findCharInCharset(c1, charset);
            if (result1) {
                auto result2 = findCharInCharset(c2, charset);
                if (result2) {
                    return result1;
                }
            }
        }
        return std::nullopt;
    }
};

