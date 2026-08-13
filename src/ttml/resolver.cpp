#include "ttml/resolver.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace arib::ttml {

namespace {

void merge_style(StyleProperties& target, const StyleProperties& source) {
#define MERGE_PROPERTY(name) if (source.name) target.name = source.name
    MERGE_PROPERTY(background_color);
    MERGE_PROPERTY(color);
    MERGE_PROPERTY(extent);
    MERGE_PROPERTY(font_family);
    MERGE_PROPERTY(font_size);
    MERGE_PROPERTY(font_style);
    MERGE_PROPERTY(font_weight);
    MERGE_PROPERTY(line_height);
    MERGE_PROPERTY(origin);
    MERGE_PROPERTY(text_decoration);
    MERGE_PROPERTY(text_outline);
    MERGE_PROPERTY(writing_mode);
    MERGE_PROPERTY(opacity);
    MERGE_PROPERTY(letter_spacing);
    MERGE_PROPERTY(ruby);
#undef MERGE_PROPERTY
}

StyleProperties inherited_style(const StyleProperties& parent) {
    StyleProperties result;
    result.color = parent.color;
    result.font_family = parent.font_family;
    result.font_size = parent.font_size;
    result.font_style = parent.font_style;
    result.font_weight = parent.font_weight;
    result.line_height = parent.line_height;
    result.text_decoration = parent.text_decoration;
    result.text_outline = parent.text_outline;
    result.writing_mode = parent.writing_mode;
    result.opacity = parent.opacity;
    result.letter_spacing = parent.letter_spacing;
    result.ruby = parent.ruby;
    return result;
}

const StyleProperties& initial_style() {
    static const StyleProperties style{
        .background_color = StyleColorValue{ { 0, 0, 0, 255 } },
        .color = StyleColorValue{ { 255, 255, 255, 255 } },
        .font_family = "round gothic",
        .font_size = StyleFontSizeValue{ 64.0, 64.0 },
        .font_style = StyleFontStyleValue::Normal,
        .font_weight = StyleFontWeightValue::Normal,
        .line_height = 64.0,
        .text_decoration = StyleTextDecorationValue::None,
        .text_outline = None{},
        .writing_mode = StyleWritingModeValue::Lrtb,
        .opacity = 1.0,
        .letter_spacing = 0.0,
    };
    return style;
}

void complete_style(StyleProperties& style) {
    const auto& initial = initial_style();
#define APPLY_INITIAL(name) if (!style.name) style.name = initial.name
    APPLY_INITIAL(background_color);
    APPLY_INITIAL(color);
    APPLY_INITIAL(font_family);
    APPLY_INITIAL(font_size);
    APPLY_INITIAL(font_style);
    APPLY_INITIAL(font_weight);
    APPLY_INITIAL(line_height);
    APPLY_INITIAL(text_decoration);
    APPLY_INITIAL(text_outline);
    APPLY_INITIAL(writing_mode);
    APPLY_INITIAL(opacity);
    APPLY_INITIAL(letter_spacing);
#undef APPLY_INITIAL
}

resolved::Timing resolve_timing(const ast::Timing& timing, const resolved::Timing& parent) {
    resolved::Timing result;
    result.begin = timing.begin
        ? std::optional{parent.begin.value_or(std::chrono::milliseconds{}) + *timing.begin}
        : parent.begin;
    result.end = timing.end
        ? std::optional{parent.begin.value_or(std::chrono::milliseconds{}) + *timing.end}
        : parent.end;
    return result;
}

class Resolver {
public:
    explicit Resolver(const ast::Document& document) : document_(document) {
        for (const auto& style : document.styles) styles_.emplace(style.id, &style);
        for (const auto& region : document.regions) regions_.emplace(region.id, &region);
    }

    ResolveResult run() {
        resolved::Document output;
        for (const auto& division : document_.divisions) {
            resolved::Division resolved_division;
            resolved_division.timing = resolve_timing(division.timing, {});

            for (const auto& paragraph : division.paragraphs) {
                resolved::Paragraph resolved_paragraph;
                resolved_paragraph.id = paragraph.id;
                resolved_paragraph.timing = resolve_timing(paragraph.timing, resolved_division.timing);

                if (!apply_refs(resolved_paragraph.style, paragraph.style_refs)) {
                    return failure();
                }

                merge_style(resolved_paragraph.style, paragraph.inline_style);
                if (!resolve_region(paragraph.region, resolved_paragraph.region)) {
                    return failure();
                }

                if (resolved_paragraph.region) {
                    auto region_style = inherited_style(resolved_paragraph.region->style);
                    merge_style(region_style, resolved_paragraph.style);
                    resolved_paragraph.style = std::move(region_style);
                }

                for (const auto& span : paragraph.spans) {
                    resolved::Span resolved_span;
                    resolved_span.id = span.id;
                    resolved_span.timing = resolve_timing(span.timing, resolved_paragraph.timing);
                    resolved_span.style = inherited_style(resolved_paragraph.style);
                    if (!apply_refs(resolved_span.style, span.style_refs)) {
                        return failure();
                    }

                    merge_style(resolved_span.style, span.inline_style);
                    if (!resolve_region(span.region, resolved_span.region)) {
                        return failure();
                    }

                    if (resolved_span.region) {
                        auto region_style = inherited_style(resolved_span.region->style);
                        merge_style(region_style, resolved_span.style);
                        resolved_span.style = std::move(region_style);
                    }
                    complete_style(resolved_span.style);
                    resolved_span.content = span.content;
                    resolved_paragraph.spans.push_back(std::move(resolved_span));
                }

                resolved_division.paragraphs.push_back(std::move(resolved_paragraph));
            }
            output.divisions.push_back(std::move(resolved_division));
        }
        return {
            .document = std::move(output),
            .error = std::nullopt
        };
    }

private:
    bool apply_refs(StyleProperties& target, const std::vector<std::string>& refs) {
        for (const auto& ref : refs) {
            StyleProperties resolved;
            if (!resolve_style(ref, resolved)) return false;
            merge_style(target, resolved);
        }
        return true;
    }

    bool resolve_style(const std::string& id, StyleProperties& output) {
        if (const auto cached = resolved_styles_.find(id); cached != resolved_styles_.end()) {
            output = cached->second;
            return true;
        }
        const auto found = styles_.find(id);
        if (found == styles_.end()) {
            error_ = ResolveError{ "unknown style reference: " + id };
            return false;
        }
        if (!resolving_styles_.emplace(id).second) {
            error_ = ResolveError{ "cyclic style reference: " + id };
            return false;
        }

        StyleProperties resolved;
        if (!apply_refs(resolved, found->second->style_refs)) {
            return false;
        }
        merge_style(resolved, found->second->style);
        resolving_styles_.erase(id);
        resolved_styles_.emplace(id, resolved);
        output = std::move(resolved);
        return true;
    }

    bool resolve_region(const std::optional<std::string>& id, std::optional<resolved::Region>& output) {
        if (!id) return true;
        const auto found = regions_.find(*id);
        if (found == regions_.end()) {
            error_ = ResolveError{ "unknown region reference: " + *id };
            return false;
        }
        StyleProperties style;
        if (!apply_refs(style, found->second->style_refs)) {
            return false;
        }
        merge_style(style, found->second->style);
        output = resolved::Region{ found->second->id, std::move(style) };
        return true;
    }

    ResolveResult failure() {
        return { .document = std::nullopt, .error = std::move(error_) };
    }

    const ast::Document& document_;
    std::unordered_map<std::string, const ast::StyleDefinition*> styles_;
    std::unordered_map<std::string, const ast::RegionDefinition*> regions_;
    std::unordered_map<std::string, StyleProperties> resolved_styles_;
    std::unordered_set<std::string> resolving_styles_;
    std::optional<ResolveError> error_;
};

} // namespace

ResolveResult resolve(const ast::Document& document) {
    return Resolver{document}.run();
}

} // namespace arib::ttml
