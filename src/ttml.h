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
#include <string_view>

struct TtmlStyleValueLength {
    TtmlStyleValueLength(float v, const std::string& u) : value(v), unit(u) {}

    float value;
    std::string unit;

};

struct TtmlStyleValueColor {
    TtmlStyleValueColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}

    static TtmlStyleValueColor parse(std::string_view text);

    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct TtmlStyleValueKeyword {
    TtmlStyleValueKeyword(const std::string& k) : keyword(k) {}

    std::string keyword;
};

struct TtmlStyleValueNumber {
    TtmlStyleValueNumber(float n) : number(n) {}

    float number;
};

class TtmlStyleValue {
public:
    using ValueType = std::variant<TtmlStyleValueLength, TtmlStyleValueColor, TtmlStyleValueKeyword, TtmlStyleValueNumber>;

private:
    ValueType value;

public:
    TtmlStyleValue(TtmlStyleValueLength length) : value(length) {}
    TtmlStyleValue(TtmlStyleValueColor color) : value(color) {}
    TtmlStyleValue(TtmlStyleValueKeyword keyword) : value(keyword) {}
    TtmlStyleValue(TtmlStyleValueNumber number) : value(number) {}

    template <typename T>
    T getValue() const {
        return std::get<T>(value);
    }

    template <typename T>
    bool isValue() const {
        return std::holds_alternative<T>(value);
    }

    template <typename T>
    void setValue(const T& new_value) {
        value = new_value;
    }
};

struct TtmlStyleFontSize {
    TtmlStyleValueLength width;
    TtmlStyleValueLength height;
};

struct TtmlStyleExtent {
    TtmlStyleValueLength width;
    TtmlStyleValueLength height;
};

struct TtmlStyleOrigin {
    TtmlStyleValueLength x;
    TtmlStyleValueLength y;
};

class TtmlStyleValueParser {
public:
    static TtmlStyleValue parse(const std::string& input);
    static TtmlStyleFontSize parseFontSize(const std::string& input);
    static std::pair<TtmlStyleValueLength, TtmlStyleValueLength> parseLengthPair(const std::string& input);

};

struct TTMLRegion {
    std::string id;
    std::optional<TtmlStyleExtent> extent;
    std::optional<TtmlStyleOrigin> origin;
};

struct TTMLStyle {
    std::string id;
    std::optional<TtmlStyleFontSize> fontSize;
    std::optional<TtmlStyleValue> lineHeight;
    std::optional<TtmlStyleValue> letterSpacing;
    std::optional<TtmlStyleValue> fontWeight;
    std::optional<TtmlStyleValue> fontStyle;
    std::optional<TtmlStyleValue> color;
    std::optional<TtmlStyleValue> backgroundColor;
    bool outlineSpecified = false;
    std::optional<TtmlStyleValue> outlineColor;
    std::optional<std::string> fontFamily;
    std::optional<TtmlStyleValue> textDecoration;
    std::optional<TtmlStyleValue> writingMode;
    std::optional<TtmlStyleValue> opacity;
    std::optional<std::string> border;
    std::optional<TtmlStyleValue> animation;
    std::optional<TtmlStyleValue> ruby;
    std::optional<std::string> textShadow;
    std::optional<TtmlStyleValue> overflow;
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
    std::optional<uint64_t> begin;
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
