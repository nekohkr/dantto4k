#include "aribUtil.h"
#include "aribEncoder.h"

namespace {

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

    {u8"\U0000FF5E", u8"\U0000301C"}, //～ 〜
    {u8"\U00002225", u8"\U00002016"}, //∥ ‖
    {u8"\U0000FFE0", u8"\U000000A2"}, //￠ ¢
    {u8"\U0000FFE1", u8"\U000000A3"}, //￡ £
    {u8"\U0000FFE2", u8"\U000000AC"}, //￢ ¬
    {u8"\U0000FFE4", u8"\U000000A6"}, //￤ ¦

    {u8"\U0000E11A", u8"〓"},
    {u8"\U00002015", u8"ー"},
};

constexpr Gaiji jisX0201KatakanaTable[] = {
    { u8"｡", u8"。" },
    { u8"｢", u8"「" },
    { u8"｣", u8"」" },
    { u8"､", u8"、" },
    { u8"･", u8"・" },
    { u8"ｦ", u8"ヲ" },
    { u8"ｧ", u8"ァ" },
    { u8"ｨ", u8"ィ" },
    { u8"ｩ", u8"ゥ" },
    { u8"ｪ", u8"ェ" },
    { u8"ｫ", u8"ォ" },
    { u8"ｬ", u8"ャ" },
    { u8"ｭ", u8"ュ" },
    { u8"ｮ", u8"ョ" },
    { u8"ｯ", u8"ッ" },
    { u8"ｰ", u8"ー" },
    { u8"ｱ", u8"ア" },
    { u8"ｲ", u8"イ" },
    { u8"ｳ", u8"ウ" },
    { u8"ｴ", u8"エ" },
    { u8"ｵ", u8"オ" },
    { u8"ｶ", u8"カ" },
    { u8"ｷ", u8"キ" },
    { u8"ｸ", u8"ク" },
    { u8"ｹ", u8"ケ" },
    { u8"ｺ", u8"コ" },
    { u8"ｻ", u8"サ" },
    { u8"ｼ", u8"シ" },
    { u8"ｽ", u8"ス" },
    { u8"ｾ", u8"セ" },
    { u8"ｿ", u8"ソ" },
    { u8"ﾀ", u8"タ" },
    { u8"ﾁ", u8"チ" },
    { u8"ﾂ", u8"ツ" },
    { u8"ﾃ", u8"テ" },
    { u8"ﾄ", u8"ト" },
    { u8"ﾅ", u8"ナ" },
    { u8"ﾆ", u8"ニ" },
    { u8"ﾇ", u8"ヌ" },
    { u8"ﾈ", u8"ネ" },
    { u8"ﾉ", u8"ノ" },
    { u8"ﾊ", u8"ハ" },
    { u8"ﾋ", u8"ヒ" },
    { u8"ﾌ", u8"フ" },
    { u8"ﾍ", u8"ヘ" },
    { u8"ﾎ", u8"ホ" },
    { u8"ﾏ", u8"マ" },
    { u8"ﾐ", u8"ミ" },
    { u8"ﾑ", u8"ム" },
    { u8"ﾒ", u8"メ" },
    { u8"ﾓ", u8"モ" },
    { u8"ﾔ", u8"ヤ" },
    { u8"ﾕ", u8"ユ" },
    { u8"ﾖ", u8"ヨ" },
    { u8"ﾗ", u8"ラ" },
    { u8"ﾘ", u8"リ" },
    { u8"ﾙ", u8"ル" },
    { u8"ﾚ", u8"レ" },
    { u8"ﾛ", u8"ロ" },
    { u8"ﾜ", u8"ワ" },
    { u8"ﾝ", u8"ン" },
    { u8"ﾞ", u8"゛" },
    { u8"ﾟ", u8"゜" }
};

void replaceSequence(std::string& str, std::string_view sequence, const char* replacement) {
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

void jisX0201KatakanaToKatakana(std::string& str) {
    // Half-width Katakana (JIS X 0201) in UTF-8 ranges from EF BD A1 to EF BE 9F.
    static const auto KatakanaReplacementTable = [] {
        std::array<const char*, 95> table{};
        for (const auto& item : jisX0201KatakanaTable) {
            const char* find = reinterpret_cast<const char*>(item.find);
            const unsigned char c1 = find[0];
            const unsigned char c2 = find[1];
            const unsigned char c3 = find[2];
            if (c1 == 0xEF && c2 == 0xBD && c3 >= 0xA1 && c3 <= 0xBF) {
                table[c3 - 0xA1] = reinterpret_cast<const char*>(item.replacement);
            } else if (c1 == 0xEF && c2 == 0xBE && c3 >= 0x80 && c3 <= 0x9F) {
                table[c3 - 0x80 + 31] = reinterpret_cast<const char*>(item.replacement);
            }
        }
        return table;
    }();

    thread_local std::string buffer;
    buffer.clear();
    buffer.reserve(str.length());

    for (size_t i = 0; i < str.length(); ) {
        const unsigned char c1 = str[i];
        if (c1 == 0xEF && i + 2 < str.length()) {
            const unsigned char c2 = str[i + 1];
            const unsigned char c3 = str[i + 2];
            if (c2 == 0xBD && c3 >= 0xA1 && c3 <= 0xBF) {
                if (KatakanaReplacementTable[c3 - 0xA1]) {
                    buffer.append(KatakanaReplacementTable[c3 - 0xA1]);
                    i += 3;
                    continue;
                }
            } else if (c2 == 0xBE && c3 >= 0x80 && c3 <= 0x9F) {
                if (KatakanaReplacementTable[c3 - 0x80 + 31]) {
                    buffer.append(KatakanaReplacementTable[c3 - 0x80 + 31]);
                    i += 3;
                    continue;
                }
            }
        }
        buffer.push_back(c1);
        i++;
    }
    str = buffer;
}

}

static std::unordered_map<std::string, std::string> aribEncodeCache;
static std::mutex aribEncodeCacheMutex;
constexpr size_t ARIB_ENCODE_CACHE_MAX_SIZE = 4096;

const std::string aribEncode(std::string_view input, bool isCaption) {
    std::string input_str{ input };
    {
        std::lock_guard<std::mutex> lock(aribEncodeCacheMutex);
        auto it = aribEncodeCache.find(input_str);
        if (it != aribEncodeCache.end()) {
            return it->second;
        }
    }

    std::string converted{ input };
    convertGaiji(converted);

    // Convert JIS X 0201 Katakana to Katakana for Mirakurun
    if (!isCaption) {
        jisX0201KatakanaToKatakana(converted);
    }

    auto result = AribEncoder::encode(converted, isCaption);

    {
        std::lock_guard<std::mutex> lock(aribEncodeCacheMutex);
        if (aribEncodeCache.size() >= ARIB_ENCODE_CACHE_MAX_SIZE) {
            aribEncodeCache.clear();
        }
        aribEncodeCache.emplace(std::move(input_str), result);
    }
    return result;
}

const std::string aribEncode(const char* input, size_t size, bool isCaption) {
    return aribEncode({ input, size }, isCaption);
}
