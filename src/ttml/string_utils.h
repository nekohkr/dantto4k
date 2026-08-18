#pragma once

#include <charconv>
#include <cmath>
#include <optional>
#include <string_view>
#include <vector>

namespace arib {

namespace ttml {

constexpr std::string_view kWhitespace = " \t\r\n";

[[nodiscard]] inline std::optional<double> string_to_double(std::string_view value) {
    if (value.empty()) {
        return {};
    }

    double result{};
    const char* const begin = value.data();
    const char* const end = begin + value.size();
    const auto [parsed_end, error] = std::from_chars(begin, end, result);

    if (error != std::errc{} || parsed_end != end || !std::isfinite(result)) {
        return {};
    }

    return result;
}

[[nodiscard]] inline std::vector<std::string_view> split_by_whitespace(std::string_view value) {
    std::vector<std::string_view> result;
    auto start = value.find_first_not_of(kWhitespace);

    while (start != std::string_view::npos) {
        const auto end = value.find_first_of(kWhitespace, start);
        if (end == std::string_view::npos) {
            result.emplace_back(value.substr(start));
            break;
        }

        result.emplace_back(value.substr(start, end - start));
        start = value.find_first_not_of(kWhitespace, end);
    }

    return result;
}

} // namespace ttml

} // namespace arib
