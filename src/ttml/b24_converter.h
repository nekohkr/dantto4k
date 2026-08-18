#pragma once

#include "ttml/resolved.h"
#include "ttml/sync_mode.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace arib {

namespace ttml {

struct B24ConvertError {
    std::string message;
};

struct B24ConvertOutput {
    std::vector<uint8_t> data;
    std::optional<std::chrono::milliseconds> begin;
    bool clear;
};

struct B24ConvertResult {
    std::vector<B24ConvertOutput> outputs;
    std::optional<B24ConvertError> error;

    [[nodiscard]] bool has_error() const noexcept {
        return error.has_value();
    }
};

[[nodiscard]] B24ConvertResult convert_to_b24(
    const resolved::Document& document,
    SyncMode mode);

} // namespace ttml

} // namespace arib
