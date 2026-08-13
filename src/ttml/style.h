#pragma once
#include <optional>
#include <string>
#include <array>
#include <variant>

namespace arib {

namespace ttml {

struct None {};

struct StyleColorValue {
    std::array<uint8_t, 4> rgba;

    constexpr std::uint8_t r() const { return rgba[0]; }
    constexpr std::uint8_t g() const { return rgba[1]; }
    constexpr std::uint8_t b() const { return rgba[2]; }
    constexpr std::uint8_t a() const { return rgba[3]; }

    friend constexpr bool operator==(const StyleColorValue&, const StyleColorValue&) = default;
};

struct LengthPair {
    double width{};
    double height{};
};

using StyleExtentValue = LengthPair;
using StyleFontSizeValue = LengthPair;

enum class StyleFontStyleValue {
    Normal,
    Italic
};

enum class StyleFontWeightValue {
    Normal,
    Bold
};

struct NormalLineHeight {};
using StyleLineHeightValue = std::variant<NormalLineHeight, double>;

enum class StyleTextDecorationValue {
    None,
    Underline
};

struct TextOutline {
    std::optional<StyleColorValue> color;
    double border_width{};
    double blur_width{};
};

using StyleTextOutlineValue = std::variant<None, TextOutline>;

enum class StyleWritingModeValue {
    Lrtb,
    Tbrl
};

struct StyleProperties {
    std::optional<StyleColorValue> background_color;
    std::optional<StyleColorValue> color;
    std::optional<StyleExtentValue> extent;
    std::optional<std::string> font_family;
    std::optional<StyleFontSizeValue> font_size;
    std::optional<StyleFontStyleValue> font_style;
    std::optional<StyleFontWeightValue> font_weight;
    std::optional<StyleLineHeightValue> line_height;
    std::optional<StyleExtentValue> origin;
    std::optional<StyleTextDecorationValue> text_decoration;
    std::optional<StyleTextOutlineValue> text_outline;
    std::optional<StyleWritingModeValue> writing_mode;
    std::optional<double> opacity;
    std::optional<double> letter_spacing;
    std::optional<std::string> ruby;
    
};

std::optional<StyleColorValue> parse_color(std::string_view text);
std::optional<double> parse_length(std::string_view value);
std::optional<LengthPair> parse_length_pair(std::string_view length_string);
std::optional<StyleTextOutlineValue> parse_text_outline(std::string_view value_string);

} // namespace ttml

} // namespace arib