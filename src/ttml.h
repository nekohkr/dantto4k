#pragma once
#include <iostream>
#include <string>
#include <regex>
#include <stdexcept>
#include <variant>
#include <memory>
#include <set>
#include <list>
#include <optional>
#include <sstream>

class TTMLCssValueLength {
public:
    TTMLCssValueLength(float v, const std::string& u) : value(v), unit(u) {}

    float value;
    std::string unit;

};

class TTMLCssValueColor {
public:
    TTMLCssValueColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}

    TTMLCssValueColor(const std::string& hex) {
        if (hex.size() != 9 || hex[0] != '#') {
            throw std::invalid_argument("Invalid color format. Expected format: #RRGGBBAA");
        }

        unsigned long value = std::stoul(hex.substr(1), nullptr, 16);
        r = static_cast<uint8_t>((value >> 24) & 0xFF);
        g = static_cast<uint8_t>((value >> 16) & 0xFF);
        b = static_cast<uint8_t>((value >> 8) & 0xFF);
        a = static_cast<uint8_t>(value & 0xFF);
    }

    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class TTMLCssValueKeyword {
public:
    TTMLCssValueKeyword(const std::string& k) : keyword(k) {}

    std::string keyword;
};

class TTMLCssValueNumber {
public:
    TTMLCssValueNumber(float n) : number(n) {}

    float number;
};

class TTMLCssValue {
public:
    using ValueType = std::variant<TTMLCssValueLength, TTMLCssValueColor, TTMLCssValueKeyword, TTMLCssValueNumber>;

private:
    ValueType value;

public:
    TTMLCssValue(TTMLCssValueLength length) : value(length) {}
    TTMLCssValue(TTMLCssValueColor color) : value(color) {}
    TTMLCssValue(TTMLCssValueKeyword keyword) : value(keyword) {}
    TTMLCssValue(TTMLCssValueNumber number) : value(number) {}

    template <typename T>
    T getValue() const {
        return std::get<T>(value);
    }

    template <typename T>
    void setValue(const T& new_value) {
        value = new_value;
    }
};

using TTMLCssValuePair = std::pair<TTMLCssValue, TTMLCssValue>;

class TTMLCssValueParser {
public:
    static TTMLCssValue parse(const std::string& input) {
        try {
            if (input.find("px") != std::string::npos || input.find("em") != std::string::npos ||
                input.find("rem") != std::string::npos || input.find("%") != std::string::npos) {
                std::regex regex(R"(^([+-]?\d*\.?\d+)(px|em|rem|%)$)");
                std::smatch match;
                if (std::regex_match(input, match, regex)) {
                    return TTMLCssValue(TTMLCssValueLength(std::stof(match[1]), match[2]));
                }
                else {
                    throw std::invalid_argument("Invalid length value: " + input);
                }
            }

            if (input.find("#") == 0) {
                return TTMLCssValue(TTMLCssValueColor(input));
            }

            static const std::set<std::string> validKeywords = { "bold", "italic", "normal", "none" };
            if (validKeywords.find(input) != validKeywords.end()) {
                return TTMLCssValue(TTMLCssValueKeyword(input));
            }

            return TTMLCssValue(TTMLCssValueNumber(std::stof(input)));

        }
        catch (const std::invalid_argument& e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    static TTMLCssValuePair parsePair(const std::string& input) {
        std::istringstream iss(input);
        std::string token1, token2;
        if (!(iss >> token1 >> token2)) {
            throw std::invalid_argument("Failed to parse TTML value pair from: " + input);
        }
        TTMLCssValue value1 = parse(token1);
        TTMLCssValue value2 = parse(token2);
        return std::make_pair(value1, value2);
    }

};

class TTMLRegion {
public:
    std::string id;
    std::optional<std::pair<TTMLCssValue, TTMLCssValue>> extent;
    std::optional<std::pair<TTMLCssValue, TTMLCssValue>> origin;
};

class TTMLStyle {
public:
    std::string id;
    std::optional<TTMLCssValuePair> fontSize;
    std::optional<TTMLCssValue> lineHeight;
    std::optional<TTMLCssValue> fontWeight;
    std::optional<TTMLCssValue> fontStyle;
    std::optional<TTMLCssValue> color;
    std::optional<TTMLCssValue> backgroundColor;
};

class TTMLSpanTag {
public:
    std::string id;
    std::string text;
	TTMLStyle style;
};

class TTMLPTag {
public:
    std::string id;
    TTMLRegion region;
    std::list<TTMLSpanTag> spanTags;
};

class TTMLDivTag {
public:
    uint64_t begin;
    std::optional<uint64_t> end;
    std::list<TTMLPTag> pTags;
};

class TTML {
public:
    std::list<TTMLDivTag> divTags;
    std::list<TTMLRegion> regions;
    std::list<TTMLStyle> styles;

};

class TTMLPaser {
public:
    static TTML parse(const std::vector<uint8_t>& input);

};