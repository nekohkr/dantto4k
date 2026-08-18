#pragma once

#include "ttml/ast.h"
#include "ttml/resolved.h"
#include "ttml/sync_mode.h"

#include <optional>
#include <string>

namespace arib {

namespace ttml {

struct ResolveError {
    std::string message;
};

struct ResolveResult {
    std::optional<resolved::Document> document;
    std::optional<ResolveError> error;

    [[nodiscard]] bool has_error() const noexcept {
        return error.has_value();
    }
};

[[nodiscard]] ResolveResult resolve(const ast::Document& document, SyncMode mode);

} // namespace ttml

} // namespace arib
