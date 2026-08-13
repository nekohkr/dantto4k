#pragma once

#include <string_view>
#include <vector>

namespace arib::ttml {

constexpr std::string_view kWhitespace = " \t\r\n";

inline std::vector<std::string_view> split_by_whitespace(std::string_view value) {
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

} // namespace arib::ttml
