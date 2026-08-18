#pragma once
#include "ttml/style.h"
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <chrono>

namespace arib {

namespace ttml {

namespace ast {

struct Timing {
    std::optional<std::chrono::milliseconds> begin;
    std::optional<std::chrono::milliseconds> end;
};

struct StyleDefinition {
    std::string id;
    std::vector<std::string> style_refs;
    StyleProperties style;
};

struct RegionDefinition {
    std::string id;
    std::vector<std::string> style_refs;
    StyleProperties style;
};

struct LineBreak {};
using SpanContent = std::variant<std::string, LineBreak>;

struct Span {
    std::optional<std::string> id;
    Timing timing;
    std::optional<std::string> region;
    std::vector<std::string> style_refs;
    std::vector<SpanContent> content;
};

struct Paragraph {
    std::optional<std::string> id;
    Timing timing;
    std::optional<std::string> region;
    std::vector<Span> spans;
};

struct Division {
    Timing timing;
    std::vector<Paragraph> paragraphs;
};

struct Document {
    std::vector<StyleDefinition> styles;
    std::vector<RegionDefinition> regions;
    std::optional<Division> division;
};

} // namespace ast

} // namespace ttml

} // namespace arib
