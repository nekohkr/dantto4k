#pragma once

#include "ttml/resolved.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace arib::ttml {

enum class PESType {
    Sync,
    Async,
};

struct B24ConvertError {
    std::string message;
};

struct B24ConvertOutput {
    std::vector<std::uint8_t> data;
    std::optional<std::chrono::milliseconds> begin;
    bool clear;
};

struct B24ConvertResult {
    std::vector<B24ConvertOutput> outputs;
    std::optional<B24ConvertError> error;

    bool has_error() const {
        return error.has_value();
    }
};

B24ConvertResult convert_to_b24(const resolved::Document& document, PESType type);

} // namespace arib::ttml
