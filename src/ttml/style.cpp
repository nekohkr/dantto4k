#include "ttml/style.h"
#include "ttml/string_utils.h"
#include <span>
#include <array>

namespace arib {

namespace ttml {

namespace {

bool parse_hex_bytes(std::string_view input, std::span<uint8_t> output) {
    if (input.size() > output.size() * 2) {
        return false;
    }

    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    for (size_t i = 0; i < output.size(); ++i) {
        const int hi = hex(input[i * 2]);
        const int lo = hex(input[i * 2 + 1]);

        if (hi < 0 || lo < 0) {
            return false;
        }

        output[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    return true;
}

}

std::optional<StyleColorValue> parse_color(std::string_view color_string) {
	if (color_string.empty()) {
		return {};
	}

	if (color_string[0] != '#') {
		return {};
	}

    StyleColorValue color;

    // #RRGGBB
	if (color_string.length() == 7) {
        if (!parse_hex_bytes(color_string.substr(1), color.rgba)) {
            return {};
        }

        color.rgba[3] = 0xFF;
	}

    // #RRGGBBAA
    if (color_string.length() == 9) {
        if (!parse_hex_bytes(color_string.substr(1), color.rgba)) {
            return {};
        }
    }

    return color;
}

std::optional<double> parse_length(std::string_view value) {
    constexpr std::string_view suffix = "px";
    if (!value.ends_with(suffix)) {
        return {};
    }

    value.remove_suffix(suffix.size());

    if (value.empty()) {
        return {};
    }

    std::string temp{ value };

    char* end = nullptr;
    const double result = std::strtod(temp.c_str(), &end);

    if (end != temp.c_str() + temp.size()) {
        return {};
    }

    return result;
}

std::optional<LengthPair> parse_length_pair(std::string_view length_string) {
    if (length_string.empty()) {
        return {};
    }

    if (kWhitespace.find(length_string.front()) != std::string_view::npos ||
        kWhitespace.find(length_string.back()) != std::string_view::npos) {
        return {};
    }

    const auto split = split_by_whitespace(length_string);
    if (split.empty() || split.size() > 2) {
        return {};
    }

    const auto width = parse_length(split[0]);
    if (!width) {
        return {};
    }

    if (split.size() == 1) {
        return LengthPair{ *width, *width };
    }

    const auto height = parse_length(split[1]);
    if (!height) {
        return {};
    }

    return LengthPair{ *width, *height };
}

std::optional<StyleTextOutlineValue> parse_text_outline(std::string_view value_string) {
    TextOutline text_outline;

    if (value_string.empty()) {
        return {};
    }

    if (value_string == "none") {
        return None{};
    }

    if (kWhitespace.find(value_string.front()) != std::string_view::npos ||
        kWhitespace.find(value_string.back()) != std::string_view::npos) {
        return {};
    }

    const auto split = split_by_whitespace(value_string);
    if (split.size() == 1) {
        const auto border_width = parse_length(split[0]);
        if (!border_width) {
            return {};
        }

        text_outline.border_width = *border_width;
        return text_outline;
    }
    else if (split.size() == 2) {
        const auto color = parse_color(split[0]);
        if (color) {
            text_outline.color = *color;

            const auto border_width = parse_length(split[1]);
            if (!border_width) {
                return {};
            }

            text_outline.border_width = *border_width;
        }
        else {
            const auto border_width = parse_length(split[0]);
            if (!border_width) {
                return {};
            }

            text_outline.border_width = *border_width;

            const auto blur_width = parse_length(split[1]);
            if (!blur_width) {
                return {};
            }

            text_outline.blur_width = *blur_width;
        }
        return text_outline;
    }
    else if (split.size() == 3) {
        const auto color = parse_color(split[0]);
        if (!color) {
            return {};
        }
        text_outline.color = *color;

        const auto border_width = parse_length(split[1]);
        if (!border_width) {
            return {};
        }

        text_outline.border_width = *border_width;

        const auto blur_width = parse_length(split[2]);
        if (!blur_width) {
            return {};
        }

        text_outline.blur_width = *blur_width;
        return text_outline;
    }

    return {};
}
} // namespace ttml

} // namespace arib
