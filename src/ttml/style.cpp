#include "ttml/style.h"
#include "ttml/string_utils.h"
#include <cstddef>
#include <span>

namespace arib {

namespace ttml {

namespace {

bool hex_to_bytes(std::string_view input, std::span<uint8_t> output) {
    if (input.size() % 2 != 0 || input.size() / 2 > output.size()) {
        return false;
    }

    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    const auto byte_count = input.size() / 2;
    for (size_t i = 0; i < byte_count; ++i) {
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
	if (color_string.length() == 7) {
        // #RRGGBB
		if (!hex_to_bytes(color_string.substr(1), color.rgba)) {
            return {};
        }

        color.rgba[3] = 0xFF;
	}
    else if (color_string.length() == 9) {
        // #RRGGBBAA
        if (!hex_to_bytes(color_string.substr(1), color.rgba)) {
            return {};
        }
    }
    else {
		return {};
    }

    return color;
}

std::optional<double> parse_length(std::string_view value) {
    constexpr std::string_view suffix = "px";
    if (!value.ends_with(suffix)) {
        return {};
    }

    value.remove_suffix(suffix.size());
    return string_to_double(value);
}

std::optional<LengthPair> parse_length_pair(std::string_view length_string, SingleLengthMode single_length_mode) {
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

    const auto x = parse_length(split[0]);
    if (!x) {
        return {};
    }

    if (split.size() == 1) {
        if (single_length_mode == SingleLengthMode::Reject) {
            return {};
        }
        return LengthPair{ *x, *x };
    }

    const auto y = parse_length(split[1]);
    if (!y) {
        return {};
    }

    return LengthPair{ *x, *y };
}

std::optional<StyleTextOutlineValue> parse_text_outline(std::string_view value_string) {
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
    if (split.empty() || split.size() > 3) {
        return {};
    }

    TextOutline text_outline;
    size_t length_index = 0;
    if (const auto color = parse_color(split.front())) {
        text_outline.color = *color;
        length_index = 1;
    }

    const auto length_count = split.size() - length_index;
    if (length_count == 0 || length_count > 2) {
        return {};
    }

    const auto border_width = parse_length(split[length_index]);
    if (!border_width) {
        return {};
    }
    text_outline.border_width = *border_width;

    if (length_count == 2) {
        const auto blur_width = parse_length(split[length_index + 1]);
        if (!blur_width) {
            return {};
        }
        text_outline.blur_width = *blur_width;
    }

    return text_outline;
}
} // namespace ttml

} // namespace arib
