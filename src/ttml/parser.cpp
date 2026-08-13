#include "parser.h"
#include "ast.h"
#include "string_utils.h"
#include "pugixml.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace arib {

namespace ttml {

namespace {

SourceLocation get_source_location(const pugi::xml_node& node, std::string_view source) {
    const auto offset = node.offset_debug();

    if (offset < 0) {
        return { 0, 1, 1 };
    }

    uint32_t line = 1;
    uint32_t column = 1;

    const auto end =
        std::min<std::size_t>(
            static_cast<std::size_t>(offset),
            source.size()
        );

    for (size_t i = 0; i < end; ++i) {
        if (source[i] == '\n') {
            ++line;
            column = 1;
        }
        else {
            ++column;
        }
    }

    return {
        .byte_offset = static_cast<size_t>(offset),
        .line = line,
        .column = column,
    };
}

std::optional<std::chrono::milliseconds> parse_time_expression(std::string_view value) {
    if (value.size() != 12 ||
        value[2] != ':' ||
        value[5] != ':' ||
        value[8] != '.') {
        return std::nullopt;
    }

    auto parse_number = [](std::string_view s) -> std::optional<int> {
        int result = 0;

        for (const char c : s) {
            if (c < '0' || c > '9') {
                return std::nullopt;
            }

            result = result * 10 + (c - '0');
        }

        return result;
    };

    const auto h = parse_number(value.substr(0, 2));
    const auto m = parse_number(value.substr(3, 2));
    const auto s = parse_number(value.substr(6, 2));
    const auto ms = parse_number(value.substr(9, 3));

    if (!h || !m || !s || !ms) {
        return std::nullopt;
    }

    if (*m >= 60 || *s >= 60) {
        return std::nullopt;
    }

    using namespace std::chrono;

    return duration_cast<milliseconds>(
        hours{ *h } +
        minutes{ *m } +
        seconds{ *s }
    ) + milliseconds{ *ms };
}

class Parser {
public:
    explicit Parser(std::string_view xml)
        : xml_(xml) {
    }

    ParseResult run() {
        ParseResult result;
        pugi::xml_document doc;
        pugi::xml_parse_result xml = doc.load_buffer(xml_.data(), xml_.size());
        if (xml.status != pugi::status_ok) {
            return ParseResult{
                .document = std::nullopt,
                .error = make_error(std::string("Malformed XML: ") + xml.description()),
            };
        }

        pugi::xml_node root;
        for (auto child = doc.first_child(); !child.empty(); child = child.next_sibling()) {
            if (child.type() == pugi::node_element) {
                root = child;
                break;
            }
        }

        const std::string_view root_name = root.name();
        if (root_name != "tt") {
            return ParseResult{
                .document = std::nullopt,
                .error = make_error("Root element must be tt", root)
            };
        }

        pugi::xml_node head;
        pugi::xml_node body;
        for (auto child = root.first_child(); !child.empty(); child = child.next_sibling()) {
            const std::string_view child_name = child.name();
            if (child_name == "head") {
                if (head.empty()) {
                    head = child;
                }
                else {
                    return ParseResult{
                        .document = std::nullopt,
                        .error = make_error("Only one head element is allowed", child)
                    };
                }
            }
            else if (child_name == "body") {
                if (body.empty()) {
                    body = child;
                }
                else {
                    return ParseResult{
                        .document = std::nullopt,
                        .error = make_error("Only one body element is allowed", child)
                    };
                }
            }
        }

        if (!head.empty()) {
            const auto error = parse_head(head);
            if (error) {
                return ParseResult{
                    .document = std::nullopt,
                    .error = error
                };
            }
        }
        if (!body.empty()) {
            const auto error = parse_body(body);
            if (error) {
                return ParseResult{
                    .document = std::nullopt,
                    .error = error
                };
            }
        }

        result.document = std::move(ast_doc_);
        return result;
    }

private:
    std::optional<ParseError> parse_body(pugi::xml_node body) {
        int div_count = 0;
        for (auto child = body.first_child(); !child.empty(); child = child.next_sibling()) {
            const std::string_view child_name = child.name();
            if (child_name == "div") {
                if (++div_count > 1) {
                    return make_error("Only one div element is allowed in body element", child);
                }

                const auto error = parse_div(child);
                if (error) {
                    return error;
                }
            }

        }

        return {};
    }

    std::optional<ParseError> parse_div(pugi::xml_node body) {
        ast::Division division;
        const auto error = parse_timing(body, division.timing);
        if (error) {
            return error;
        }

        for (auto child = body.first_child(); !child.empty(); child = child.next_sibling()) {
            const std::string_view child_name = child.name();
            if (child_name == "p") {
                const auto error = parse_p(child, division);
                if (error) {
                    return error;
                }
            }
        }

        ast_doc_.divisions.push_back(division);

        return {};
    }

    std::optional<ParseError> parse_p(pugi::xml_node node, ast::Division& division) {
        ast::Paragraph paragraph;

        if (const auto id_attr = node.attribute("xml:id")) {
            const std::string_view id = id_attr.value();
            if (id.empty()) {
                return make_error("xml:id must not be empty", node);
            }

            if (!register_id(id)) {
                return make_error("duplicate xml:id", node);
            }

            paragraph.id = id;
        }

        const auto error = parse_timing(node, paragraph.timing);
        if (error) {
            return error;
        }

        if (const auto error = parse_region_ref(node, paragraph.region)) {
            return error;
        }

        if (const auto error = parse_style_refs(node, paragraph.style_refs)) {
            return error;
        }
        if (const auto error = parse_style_properties(node, paragraph.inline_style)) {
            return error;
        }

        for (auto child = node.first_child(); !child.empty(); child = child.next_sibling()) {
            const std::string_view child_name = child.name();
            if (child_name == "span") {
                const auto error = parse_span(child, paragraph);
                if (error) {
                    return error;
                }
            }
        }

        division.paragraphs.push_back(std::move(paragraph));

        return {};
    }

    std::optional<ParseError> parse_span(pugi::xml_node node, ast::Paragraph& paragraph) {
        ast::Span span;

        if (const auto id_attr = node.attribute("xml:id")) {
            const std::string_view id = id_attr.value();
            if (id.empty()) {
                return make_error("xml:id must not be empty", node);
            }
            if (!register_id(id)) {
                return make_error("duplicate xml:id", node);
            }
            span.id = id;
        }

        if (const auto error = parse_timing(node, span.timing)) {
            return error;
        }
        if (const auto error = parse_region_ref(node, span.region)) {
            return error;
        }
        if (const auto error = parse_style_refs(node, span.style_refs)) {
            return error;
        }
        if (const auto error = parse_style_properties(node, span.inline_style)) {
            return error;
        }

        for (auto child = node.first_child(); !child.empty(); child = child.next_sibling()) {
            switch (child.type()) {
            case pugi::node_pcdata:
            case pugi::node_cdata:
                span.content.emplace_back(std::string(child.value()));
                break;

            case pugi::node_element:
                if (std::string_view(child.name()) != "br") {
                    return make_error("span element may only contain text and br elements", child);
                }
                if (child.first_child()) {
                    return make_error("br element must be empty", child);
                }
                span.content.emplace_back(ast::LineBreak{});
                break;

            default:
                return make_error("span element may only contain text and br elements", child);
            }
        }

        paragraph.spans.push_back(std::move(span));

        return {};
    }

    std::optional<ParseError> parse_head(pugi::xml_node head) {
        for (auto child = head.first_child(); !child.empty(); child = child.next_sibling()) {
            const std::string_view child_name = child.name();
            if (child_name == "styling") {
                const auto error = parse_styling(child);
                if (error) {
                    return error;
                }
            }
            else if (child_name == "layout") {
                const auto error = parse_layout(child);
                if (error) {
                    return error;
                }
            }
        }

        return {};
    }

    std::optional<ParseError> parse_styling(pugi::xml_node styling) {
        for (auto child = styling.first_child(); !child.empty(); child = child.next_sibling()) {
            const std::string_view child_name = child.name();
            if (child_name != "style") {
                continue;
            }

            const auto id_attr = child.attribute("xml:id");
            if (!id_attr) {
                return make_error("style element requires xml:id", child);
            }

            const std::string_view id = id_attr.value();
            if (id.empty()) {
                return make_error("xml:id must not be empty", child);
            }

            if (!register_id(id)) {
                return make_error("duplicate xml:id", child);
            }

            ast::StyleDefinition definition;
            definition.id = id;
            const auto refs_error = parse_style_refs(child, definition.style_refs);
            if (refs_error) {
                return refs_error;
            }
            const auto error = parse_style_properties(child, definition.style);
            if (error) {
                return error;
            }
            ast_doc_.styles.push_back(std::move(definition));
        }

        return {};
    }

    std::optional<ParseError> parse_style_properties(pugi::xml_node style, StyleProperties& output, bool is_region=false) {
        for (auto attr = style.first_attribute(); !attr.empty(); attr = attr.next_attribute()) {
            const std::string_view attr_name = attr.name();
            const std::string_view attr_value = attr.value();
            
            if (attr_name == "tts:backgroundColor") {
                const auto color = parse_color(attr_value);
                if (!color) {
                    return make_error("invalid tts:backgroundColor value", style);
                }
                output.background_color = color;
            }
            else if (attr_name == "tts:color") {
                const auto color = parse_color(attr_value);
                if (!color) {
                    return make_error("invalid tts:color value", style);
                }
                output.color = color;
            }
            else if (attr_name == "tts:extent" && is_region) {
                const auto extent = parse_length_pair(attr_value);
                if (!extent) {
                    return make_error("invalid tts:extent value", style);
                }
                output.extent = extent;
            }
            else if (attr_name == "tts:fontFamily") {
                if (attr_value.empty()) {
                    return make_error("invalid tts:fontFamily value", style);
                }
                output.font_family = attr_value;
            }
            else if (attr_name == "tts:fontSize") {
                const auto font_size = parse_length_pair(attr_value);
                if (!font_size) {
                    return make_error("invalid tts:fontSize value", style);
                }
                output.font_size = font_size;
            }
            else if (attr_name == "tts:fontStyle") {
                if (attr_value == "normal") {
                    output.font_style = StyleFontStyleValue::Normal;
                }
                else if (attr_value == "italic") {
                    output.font_style = StyleFontStyleValue::Italic;
                }
                else {
                    return make_error("invalid tts:fontStyle value", style);
                }
            }
            else if (attr_name == "tts:fontWeight") {
                if (attr_value == "normal") {
                    output.font_weight = StyleFontWeightValue::Normal;
                }
                else if (attr_value == "bold") {
                    output.font_weight = StyleFontWeightValue::Bold;
                }
                else {
                    return make_error("invalid tts:fontWeight value", style);
                }
            }
            else if (attr_name == "tts:lineHeight") {
                if (attr_value == "normal") {
                    output.line_height = NormalLineHeight{};
                }
                else {
                    const auto line_height = parse_length(attr_value);
                    if (!line_height) {
                        return make_error("invalid tts:lineHeight value", style);
                    }
                    output.line_height = *line_height;
                }
            }
            else if (attr_name == "tts:origin" && is_region) {
                const auto origin = parse_length_pair(attr_value);
                if (!origin) {
                    return make_error("invalid tts:origin value", style);
                }
                output.origin = origin;
            }
            else if (attr_name == "tts:textDecoration") {
                if (attr_value == "none") {
                    output.text_decoration = StyleTextDecorationValue::None;
                }
                else if (attr_value == "underline") {
                    output.text_decoration = StyleTextDecorationValue::Underline;
                }
                else {
                    return make_error("invalid tts:textDecoration value", style);
                }
            }
            else if (attr_name == "tts:textOutline") {
                const auto text_outline = parse_text_outline(attr_value);
                if (!text_outline) {
                    return make_error("invalid tts:textOutline value", style);
                }

                output.text_outline = text_outline;
            }
            else if (attr_name == "tts:writingMode") {
                if (attr_value == "lrtb") {
                    output.writing_mode = StyleWritingModeValue::Lrtb;
                }
                else if (attr_value == "tbrl") {
                    output.writing_mode = StyleWritingModeValue::Tbrl;
                }
                else {
                    return make_error("invalid tts:writingMode value", style);
                }
            }
            else if (attr_name == "tts:opacity") {
                const auto opacity = parse_length(attr_value);
                if (!opacity) {
                    return make_error("invalid tts:opacity value", style);
                }
                if (*opacity < 0 || *opacity > 1) {
                    return make_error("invalid tts:opacity value", style);
                }

                output.opacity = opacity;
            }
            else if (attr_name == "arib-tt:letter-spacing") {
                const auto letter_spacing = parse_length(attr_value);
                if (!letter_spacing) {
                    return make_error("invalid arib-tt:letter-spacing value", style);
                }
                if (*letter_spacing < 0) {
                    return make_error("invalid arib-tt:letter-spacing value", style);
                }

                output.letter_spacing = letter_spacing;
            }
            else if (attr_name == "arib-tt:ruby") {
                if (attr_value.empty()) {
                    return make_error("invalid arib-tt:ruby value", style);
                }

                output.ruby = attr_value;
            }

        }

        return {};
    }

    std::optional<ParseError> parse_style_refs(
        pugi::xml_node node,
        std::vector<std::string>& output) {
        if (const auto style_attr = node.attribute("style")) {
            const auto refs = split_by_whitespace(style_attr.value());
            if (refs.empty()) {
                return make_error("style must not be empty", node);
            }
            for (const auto ref : refs) {
                output.emplace_back(ref);
            }
        }
        return {};
    }

    std::optional<ParseError> parse_region_ref(
        pugi::xml_node node,
        std::optional<std::string>& output) {
        const auto region_attr = node.attribute("region");
        if (!region_attr) {
            return {};
        }

        const std::string_view region = region_attr.value();
        const auto refs = split_by_whitespace(region);
        if (refs.size() != 1 || refs.front() != region) {
            return make_error("region must contain exactly one region reference without surrounding whitespace", node);
        }

        output = region;
        return {};
    }

    std::optional<ParseError> parse_layout(pugi::xml_node layout) {
        for (auto child = layout.first_child(); !child.empty(); child = child.next_sibling()) {
            const std::string_view child_name = child.name();
            if (child_name != "region") {
                return make_error("", child);
            }

            const auto id_attr = child.attribute("xml:id");
            if (!id_attr) {
                return make_error("style element requires xml:id", child);
            }

            const std::string_view id = id_attr.value();
            if (id.empty()) {
                return make_error("xml:id must not be empty", child);
            }

            if (!register_id(id)) {
                return make_error("duplicate xml:id", child);
            }

            ast::RegionDefinition definition;
            definition.id = id;
            const auto refs_error = parse_style_refs(child, definition.style_refs);
            if (refs_error) {
                return refs_error;
            }
            const auto error = parse_style_properties(child, definition.style, true);
            if (error) {
                return error;
            }
            ast_doc_.regions.push_back(std::move(definition));
        }

        return {};
    }

    std::optional<ParseError> parse_timing(pugi::xml_node node, ast::Timing& timing) {
        if (const auto begin_attr = node.attribute("begin")) {
            auto begin = parse_time_expression(begin_attr.value());
            if (!begin) {
                return make_error("invalid begin time", node);
            }

            timing.begin = *begin;
        }

        if (const auto end_attr = node.attribute("end")) {
            auto end = parse_time_expression(end_attr.value());
            if (!end) {
                return make_error("invalid end time", node);
            }

            timing.end = *end;
        }

        return {};
    }

    bool register_id(std::string_view id) {
        if (!ids_.emplace(id).second) {
            return false;
        }

        return true;
    }

    ParseError make_error(std::string message) const {
        return {
            .location = {0, 1, 1},
            .message = std::move(message),
        };
    }

    ParseError make_error(
        std::string message,
        const pugi::xml_node& node) const {
        return {
            .location = get_source_location(node, xml_),
            .message = std::move(message),
        };
    }

private:
    std::string xml_;
    ast::Document ast_doc_;
    std::unordered_set<std::string> ids_;
};

}

ParseResult parse(std::string_view xml) {
	return Parser{ xml }.run();
}

}

}
