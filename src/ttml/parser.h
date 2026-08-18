#pragma once
#include "ttml/ast.h"
#include "ttml/sync_mode.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace arib {

namespace ttml {

struct SourceLocation {
    size_t byte_offset{};
    uint32_t line{};
    uint32_t column{};
};

struct ParseError {
    SourceLocation location;
    std::string message;
};

struct ParseResult {
    std::optional<ast::Document> document;
    std::optional<ParseError> error;

    [[nodiscard]] bool has_error() const noexcept {
        return error.has_value();
    }
};

[[nodiscard]] ParseResult parse(std::string_view xml, SyncMode mode);

} // namespace ttml

} // namespace arib
