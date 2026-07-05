#include "aribCharsetEncoder.h"
#include "aribCharsetTables.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace arib::charset::detail {

struct CharLookupResult {
    const Charset* charset;
    uint8_t row;
    uint8_t col;
};

using CharLookupTable = std::unordered_map<char32_t, std::vector<CharLookupResult>>;

CharLookupTable buildCharLookupTable() {
    CharLookupTable table;
    auto charsets = lookupCharsets();

    for (const auto* charset : charsets) {
        for (uint8_t row = 0; row < charset->rowCount; row++) {
            if (charset->rowIndex) {
                if (charset->rowIndex[row] == -1) {
                    continue;
                }
            }

            for (uint8_t col = 0; col < 94; col++) {
                const uint8_t actualRow = charset->rowIndex ? charset->rowIndex[row] : row;
                char32_t c = charset->rows[actualRow][col];

                CharLookupResult result = {
                    charset,
                    static_cast<uint8_t>(row + charset->rowStart),
                    col
                };
                table[c].push_back(result);
            }
        }

        if (charset->code == CharsetCode::AdditionalSymbols) {
            for (const auto& alias : additionalSymbolAliases()) {
                const Charset* charset = findCharset(alias.target.charset);
                if (charset == nullptr) {
                    continue;
                }

                const uint8_t rowEnd = charset->rowStart + charset->rowCount;
                if (alias.target.row < charset->rowStart || alias.target.row >= rowEnd || alias.target.col >= 94) {
                    continue;
                }

                table[alias.codepoint].push_back({
                    charset,
                    alias.target.row,
                    alias.target.col,
                });
            }
        }
    }

    return table;
}

const CharLookupTable& charLookupTable() {
    static const CharLookupTable table = buildCharLookupTable();
    return table;
}

std::optional<CharLookupResult> findChar(char32_t c, CharsetCode candidate1, CharsetCode candidate2) {
    const auto& table = charLookupTable();
    auto it = table.find(c);
    if (it == table.end()) {
        return std::nullopt;
    }

    const auto& results = it->second;
    if (results.empty()) {
        return std::nullopt;
    }

    if (results.size() == 1) {
        return results[0];
    }

    if (candidate1 != CharsetCode::None &&
        candidate1 != CharsetCode::JISKanjiPlane1 &&
        candidate1 != CharsetCode::JISKanjiPlane2) {
        for (const auto& res : results) {
            if (res.charset->code == candidate1) return res;
        }
    }

    if (candidate2 != CharsetCode::None &&
        candidate2 != CharsetCode::JISKanjiPlane1 &&
        candidate2 != CharsetCode::JISKanjiPlane2) {
        for (const auto& res : results) {
            if (res.charset->code == candidate2) return res;
        }
    }

    return results[0];
}

std::optional<CharLookupResult> findCharsetBy2Char(char32_t c1, char32_t c2, CharsetCode candidate1, CharsetCode candidate2) {
    const auto& table = charLookupTable();
    auto it1 = table.find(c1);
    if (it1 == table.end()) return std::nullopt;
    const auto& results1 = it1->second;

    auto it2 = table.find(c2);
    if (it2 == table.end()) return std::nullopt;
    const auto& results2 = it2->second;

    auto hasCharset = [](const std::vector<CharLookupResult>& list, const Charset* target) {
        for (const auto& res : list) {
            if (res.charset == target) return true;
        }
        return false;
    };

    if (candidate1 != CharsetCode::None &&
        candidate1 != CharsetCode::JISKanjiPlane1 &&
        candidate1 != CharsetCode::JISKanjiPlane2) {
        const Charset* cs = findCharset(candidate1);
        if (cs != nullptr) {
            bool found1 = false;
            CharLookupResult res1{};
            for (const auto& r : results1) {
                if (r.charset == cs) {
                    found1 = true;
                    res1 = r;
                    break;
                }
            }

            if (found1 && hasCharset(results2, cs)) {
                return res1;
            }
        }
    }

    if (candidate2 != CharsetCode::None &&
        candidate2 != CharsetCode::JISKanjiPlane1 &&
        candidate2 != CharsetCode::JISKanjiPlane2) {
        const Charset* cs = findCharset(candidate2);
        if (cs != nullptr) {
            bool found1 = false;
            CharLookupResult res1{};
            for (const auto& r : results1) {
                if (r.charset == cs) {
                    found1 = true;
                    res1 = r;
                    break;
                }
            }

            if (found1 && hasCharset(results2, cs)) {
                return res1;
            }
        }
    }

    for (const auto& res1 : results1) {
        if (hasCharset(results2, res1.charset)) {
            return res1;
        }
    }

    return std::nullopt;
}

class Encoder {
public:
    explicit Encoder(EncodeMode encodeMode)
        : mode(encodeMode) {
        if (mode == EncodeMode::Caption) {
            state.graphic = { CharsetCode::JISKanjiPlane1, CharsetCode::Alphanumeric, CharsetCode::Hiragana, CharsetCode::Macro };
        }
        else {
            state.graphic = { CharsetCode::JISKanjiPlane1, CharsetCode::Alphanumeric, CharsetCode::Hiragana, CharsetCode::Katakana };
        }
    }

    std::string encode(std::u8string_view input) {
        return encodeImpl(std::u8string(input));
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


    static inline const char32_t fullwidthChars[] = {
        0xFF01, 0x201D, 0xFF03, 0xFF04, 0xFF05, 0xFF06, 0x2019, 0xFF08,
        0xFF09, 0xFF0A, 0xFF0B, 0xFF0C, 0xFF0D, 0xFF0E, 0xFF0F, 0xFF10,
        0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17, 0xFF18,
        0xFF19, 0xFF1A, 0xFF1B, 0xFF1C, 0xFF1D, 0xFF1E, 0xFF1F, 0xFF20,
        0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28,
        0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F, 0xFF30,
        0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38,
        0xFF39, 0xFF3A, 0xFF3B, 0xFFE5, 0xFF3D, 0xFF3E, 0xFF3F, 0x2018,
        0xFF41, 0xFF42, 0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48,
        0xFF49, 0xFF4A, 0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F, 0xFF50,
        0xFF51, 0xFF52, 0xFF53, 0xFF54, 0xFF55, 0xFF56, 0xFF57, 0xFF58,
        0xFF59, 0xFF5A, 0xFF5B, 0xFF5C, 0xFF5D, 0xFFE3, 0,
    };
    static inline const char32_t halfwidthChars[] = U"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

    
    struct State {
        std::array<CharsetCode, 4> graphic{};
        bool isGR{false};
        uint8_t gl{0};
        uint8_t gr{2};
        uint8_t singleShift{0};
        CharsetCode prevCharsetCode{CharsetCode::None};
        uint8_t characterSize{NSZ};
        std::string output;
        std::u32string u32;
    };

    const EncodeMode mode;
    State state;

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
        state.u32 = toU32(input);
        size_t pos = 0;
        for (char32_t c : state.u32) {
            bool fullwidth = isFullwidth(c);
            if (fullwidth) {
                c = toHalfwidth(c);
            }

            if (c == '\0') {
                state.output.push_back(0);
                ++pos;
                continue;
            }

            if (c == '\n') {
                state.output.push_back(APR);
                ++pos;
                continue;
            }

            if (c == ' ') {
                if (mode != EncodeMode::Caption) {
                    if (fullwidth) {
                        setCharacterSize(NSZ);
                    }
                    else {
                        setCharacterSize(MSZ);
                    }
                }
                state.output.push_back(SP);
                ++pos;
                continue;
            }

            CharsetCode glCode = state.graphic[state.gl];
            CharsetCode grCode = state.graphic[state.gr];
            auto find = findChar(c, glCode, grCode);

            if (!find) {
                ++pos;
                continue;
            }

            // Charset is not in GL or GR
            if (state.u32.size() > pos + 1 &&
                find->charset->code != glCode && find->charset->code != grCode) {
                // If the character is neither Hiragana nor Katakana nor AdditionalSymbol
                if (!(c >= 0x3041 && c <= 0x3093) && !(c >= 0x30A1 && c <= 0x30F6) && find->charset->code != CharsetCode::AdditionalSymbols) {
                    // Check if the current and next characters belong to the same charset
                    auto find2 = findCharsetBy2Char(c, state.u32[pos + 1], glCode, grCode);
                    if (find2) {
                        find = find2;
                    }
                }
            }

            if (mode != EncodeMode::Caption) {
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

            selectCharset(find->charset, pos);

            const uint8_t mask = state.isGR ? 0x80 : 0x00;
            if (find->charset->is2Byte) {
                state.output.push_back((find->row + 0x21) | mask);
            }
            state.output.push_back((find->col + 0x21) | mask);
            ++pos;
        }
        return state.output;
    }

    void setCharacterSize(uint8_t size) {
        if (state.characterSize != size) {
            state.output.push_back(size);
            state.characterSize = size;
        }
    }

    void selectCharset(const Charset* charset, size_t pos) {
        uint8_t graphicIndex = findGraphicByCharsetCode(charset->code);

        // Check single shift
        if (charset->code == CharsetCode::Hiragana || charset->code == CharsetCode::Katakana) {
            if (state.prevCharsetCode != charset->code) {
                if (getCharsetRunLength(charset->code, pos) <= 2) {
                    if (graphicIndex == 2 || graphicIndex == 3) {
                        state.singleShift = graphicIndex;
                    }
                    else {
                        state.singleShift = 0;
                    }
                }
                else {
                    state.singleShift = 0;
                }
            }
        }
        else {
            state.singleShift = 0;
        }

        if (state.gl == graphicIndex) {
            state.isGR = false;
            state.prevCharsetCode = charset->code;
            return;
        }
        if (state.gr == graphicIndex) {
            state.isGR = true;
            state.prevCharsetCode = charset->code;
            return;
        }

        if (state.singleShift) {
            state.output.push_back(state.singleShift == 2 ? SS2 : SS3);
            state.isGR = false;
            state.prevCharsetCode = charset->code;
            return;
        }

        if (graphicIndex != 0xFF) {
            if (charset->code == CharsetCode::JISX0201Katakana) {
                setGR(graphicIndex);
            }
            else if (graphicIndex == 0) {
                setGL(graphicIndex);
            }
            else {
                // Check the next charset to decide whether to use GL or GR
                CharsetCode glCode = state.graphic[state.gl];
                CharsetCode grCode = state.graphic[state.gr];

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
        }
        else {
            // charset is not in G0-G3
            if (charset->code == CharsetCode::JISX0201Katakana) {
                setGraphicToCharset(1, charset->code);
                setGR(1);
            }
            else {
                setGraphicToCharset(0, charset->code);
                setGL(0);
            }
        }

        state.prevCharsetCode = charset->code;
    }

    void setGraphicToCharset(uint8_t graphicIndex, CharsetCode charsetCode) {
        if (graphicIndex > 3) {
            return;
        }

        const Charset* charset = findCharset(charsetCode);
        if (charset == nullptr) {
            return;
        }

        state.output.push_back(ESC);
        if (charset->is2Byte == false) {
            state.output.push_back(0x28 + graphicIndex);
            state.output.push_back(static_cast<uint8_t>(charsetCode));
        }
        else {
            state.output.push_back(0x24);
            if (graphicIndex >= 1 && graphicIndex <= 3) {
                state.output.push_back(0x29 + graphicIndex);
            }
            state.output.push_back(static_cast<uint8_t>(charsetCode));
        }

        state.graphic[graphicIndex] = static_cast<CharsetCode>(charsetCode);
    }

    void setGL(uint8_t graphicIndex) {
        state.isGR = false;
        if (state.gl == graphicIndex) {
            return;
        }
        state.gl = graphicIndex;

        if (graphicIndex == 0) {
            state.output.push_back(LS0);
        }
        else if (graphicIndex == 1) {
            state.output.push_back(LS1);
        }
        else if (graphicIndex == 2) {
            state.output.push_back(ESC);
            state.output.push_back(0x6E);
        }
        else if (graphicIndex == 3) {
            state.output.push_back(ESC);
            state.output.push_back(0x6F);
        }
    }

    void setGR(uint8_t graphicIndex) {
        state.isGR = true;
        if (state.gr == graphicIndex) {
            return;
        }
        state.gr = graphicIndex;

        if (graphicIndex == 0) {
        }
        else if (graphicIndex == 1) {
            state.output.push_back(ESC);
            state.output.push_back(0x7E);
        }
        else if (graphicIndex == 2) {
            state.output.push_back(ESC);
            state.output.push_back(0x7D);
        }
        else if (graphicIndex == 3) {
            state.output.push_back(ESC);
            state.output.push_back(0x7C);
        }
    }

    uint8_t findGraphicByCharsetCode(CharsetCode charsetCode) {
        for (uint8_t i = 0; i < 4; i++) {
            if (state.graphic[i] == charsetCode) {
                return i;
            }
        }

        return 0xFF;
    }

    size_t getCharsetRunLength(CharsetCode charsetCode, size_t pos) {
        size_t count = 0;
        for (size_t i = pos; i < state.u32.size(); i++) {
            auto result = findChar(state.u32[i], charsetCode, CharsetCode::None);
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
        for (size_t i = pos; i < state.u32.size(); i++) {
            auto result = findChar(state.u32[i], currentCharsetCode, CharsetCode::None);
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

};

} // namespace arib::charset::detail

namespace arib::charset {

std::string encode(std::string_view input, EncodeMode mode) {
    detail::Encoder encoder(mode);
    std::u8string utf8(reinterpret_cast<const char8_t*>(input.data()), input.size());
    return encoder.encode(utf8);
}

std::string encode(std::u8string_view input, EncodeMode mode) {
    detail::Encoder encoder(mode);
    return encoder.encode(input);
}

} // namespace arib::charset
