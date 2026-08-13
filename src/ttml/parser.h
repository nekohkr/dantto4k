#pragma once
#include "ttml/style.h"
#include "ttml/ast.h"

namespace arib {

namespace ttml {

struct SourceLocation {
    std::size_t byte_offset{};
    std::uint32_t line{};
    std::uint32_t column{};
};

struct ParseError {
    SourceLocation location;
    std::string message;
};

struct ParseResult {
    std::optional<ast::Document> document;
    std::optional<ParseError> error;

    bool has_error() const {
        return error.has_value();
    }
};

ParseResult parse(std::string_view xml);

} // namespace ttml

} // namespace arib