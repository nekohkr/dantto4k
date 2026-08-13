#pragma once

#include "ttml/ast.h"
#include "ttml/resolved.h"

#include <optional>
#include <string>

namespace arib::ttml {

struct ResolveError {
    std::string message;
};

struct ResolveResult {
    std::optional<resolved::Document> document;
    std::optional<ResolveError> error;

    bool has_error() const {
        return error.has_value();
    }
};

ResolveResult resolve(const ast::Document& document);

} // namespace arib::ttml
