#pragma once

#include "ttml/ast.h"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace arib {

namespace ttml {

namespace resolved {

struct Region {
    std::string id;
    StyleProperties style;
};

struct Timing {
    std::optional<std::chrono::milliseconds> begin;
    std::optional<std::chrono::milliseconds> end;
};

struct Span {
    std::optional<std::string> id;
    Timing timing;
    StyleProperties style;
    std::optional<Region> region;
    std::vector<ast::SpanContent> content;
};

struct Paragraph {
    std::optional<std::string> id;
    Timing timing;
    StyleProperties style;
    std::optional<Region> region;
    std::vector<Span> spans;
};

struct Division {
    Timing timing;
    std::vector<Paragraph> paragraphs;
};

struct Document {
    std::optional<Division> division;
};

} // namespace arib:

} // namespace ttml

} // namespace resolved
