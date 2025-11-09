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
    static TTMLCssValue parse(const std::string& input);
    static TTMLCssValuePair parsePair(const std::string& input);

};

struct TTMLRegion {
    std::string id;
    std::optional<std::pair<TTMLCssValue, TTMLCssValue>> extent;
    std::optional<std::pair<TTMLCssValue, TTMLCssValue>> origin;
};

struct TTMLStyle {
    std::string id;
    std::optional<TTMLCssValuePair> fontSize;
    std::optional<TTMLCssValue> lineHeight;
    std::optional<TTMLCssValue> fontWeight;
    std::optional<TTMLCssValue> fontStyle;
    std::optional<TTMLCssValue> color;
    std::optional<TTMLCssValue> backgroundColor;
};

struct TTMLSpanTag {
    std::string id;
    std::string text;
	TTMLStyle style;
};

struct TTMLPTag {
    std::string id;
    TTMLRegion region;
    std::list<TTMLSpanTag> spanTags;
};

struct TTMLDivTag {
    uint64_t begin;
    std::optional<uint64_t> end;
    std::list<TTMLPTag> pTags;
};

struct TTML {
    std::list<TTMLDivTag> divTags;
    std::list<TTMLRegion> regions;
    std::list<TTMLStyle> styles;

};

class TTMLPaser {
public:
    static TTML parse(const std::string& input);

};