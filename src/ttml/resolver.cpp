#include "ttml/resolver.h"

#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <utility>

namespace arib {

namespace ttml {

namespace {

template<typename T>
void merge_property(std::optional<T>& target, const std::optional<T>& source) {
    if (source) {
        target = source;
    }
}

template<typename T>
void apply_initial(std::optional<T>& target, const std::optional<T>& initial) {
    if (!target) {
        target = initial;
    }
}

void merge_style(StyleProperties& target, const StyleProperties& source) {
    merge_property(target.background_color, source.background_color);
    merge_property(target.color, source.color);
    merge_property(target.extent, source.extent);
    merge_property(target.font_family, source.font_family);
    merge_property(target.font_size, source.font_size);
    merge_property(target.font_style, source.font_style);
    merge_property(target.font_weight, source.font_weight);
    merge_property(target.line_height, source.line_height);
    merge_property(target.origin, source.origin);
    merge_property(target.text_decoration, source.text_decoration);
    merge_property(target.text_outline, source.text_outline);
    merge_property(target.writing_mode, source.writing_mode);
    merge_property(target.letter_spacing, source.letter_spacing);
    merge_property(target.ruby, source.ruby);
}

StyleProperties inherited_style(const StyleProperties& parent) {
    StyleProperties result;
    result.background_color = parent.background_color;
    result.color = parent.color;
    result.font_family = parent.font_family;
    result.font_size = parent.font_size;
    result.font_style = parent.font_style;
    result.font_weight = parent.font_weight;
    result.line_height = parent.line_height;
    result.text_decoration = parent.text_decoration;
    result.text_outline = parent.text_outline;
    result.writing_mode = parent.writing_mode;
    result.letter_spacing = parent.letter_spacing;
    result.ruby = parent.ruby;
    return result;
}

const StyleProperties& initial_style() {
    static const StyleProperties style{
        .background_color = StyleColorValue{ { 0, 0, 0, 0 } },
        .color = StyleColorValue{ { 255, 255, 255, 255 } },
        .font_family = "round gothic",
        .font_style = StyleFontStyleValue::Normal,
        .font_weight = StyleFontWeightValue::Normal,
        .line_height = NormalLineHeight{},
        .text_decoration = StyleTextDecorationValue::None,
        .text_outline = None{},
        .writing_mode = StyleWritingModeValue::Lrtb,
        .letter_spacing = 0.0,
    };
    return style;
}

void complete_style(StyleProperties& style) {
    const auto& initial = initial_style();
    apply_initial(style.background_color, initial.background_color);
    apply_initial(style.color, initial.color);
    apply_initial(style.font_family, initial.font_family);
    apply_initial(style.font_size, initial.font_size);
    apply_initial(style.font_style, initial.font_style);
    apply_initial(style.font_weight, initial.font_weight);
    apply_initial(style.line_height, initial.line_height);
    apply_initial(style.text_decoration, initial.text_decoration);
    apply_initial(style.text_outline, initial.text_outline);
    apply_initial(style.writing_mode, initial.writing_mode);
    apply_initial(style.letter_spacing, initial.letter_spacing);
}

struct TimingResolution {
    resolved::Timing timing;
    bool has_error{false};
};

TimingResolution resolve_timing(const ast::Timing& timing, const resolved::Timing& parent, SyncMode mode) {
    if (mode == SyncMode::Async) {
        return {};
    }
    resolved::Timing resolved_timing;
    resolved_timing.begin = timing.begin
        ? std::optional{parent.begin.value_or(std::chrono::milliseconds{}) + *timing.begin}
        : parent.begin;
    resolved_timing.end = timing.end
        ? std::optional{parent.begin.value_or(std::chrono::milliseconds{}) + *timing.end}
        : parent.end;

    if (parent.end) {
        if (resolved_timing.begin && *resolved_timing.begin >= *parent.end) {
            return {
                .timing = std::move(resolved_timing),
                .has_error = true
            };
        }
        if (resolved_timing.end && *resolved_timing.end > *parent.end) {
            resolved_timing.end = parent.end;
        }
    }

    const auto effective_begin = resolved_timing.begin.value_or(std::chrono::milliseconds{});
    if (resolved_timing.end && effective_begin >= *resolved_timing.end) {
        return {
            .timing = std::move(resolved_timing),
            .has_error = true
        };
    }

    return {
        .timing = std::move(resolved_timing)
    };
}

class Resolver {
public:
    Resolver(const ast::Document& document, SyncMode mode)
        : document_(document), mode_(mode) {
        styles_.reserve(document.styles.size());
        regions_.reserve(document.regions.size());
        resolved_styles_.reserve(document.styles.size());
        resolving_styles_.reserve(document.styles.size());

        for (const auto& style : document.styles) {
            styles_.emplace(std::string_view{style.id}, &style);
        }
        for (const auto& region : document.regions) {
            regions_.emplace(std::string_view{region.id}, &region);
        }
    }

    ResolveResult run() {
        resolved::Document output;
        if (!document_.division) {
            return {
                .document = std::move(output)
            };
        }

        const auto& division = *document_.division;
        auto division_timing = resolve_timing(division.timing, {}, mode_);
        if (division_timing.has_error) {
            return {
                .document = std::move(output)
            };
        }

        resolved::Division resolved_division;
        resolved_division.timing = std::move(division_timing.timing);
        resolved_division.paragraphs.reserve(division.paragraphs.size());

        for (const auto& paragraph : division.paragraphs) {
            auto paragraph_timing = resolve_timing(paragraph.timing, resolved_division.timing, mode_);
            if (paragraph_timing.has_error) {
                continue;
            }

            resolved::Paragraph resolved_paragraph;
            resolved_paragraph.id = paragraph.id;
            resolved_paragraph.timing = std::move(paragraph_timing.timing);
            resolved_paragraph.spans.reserve(paragraph.spans.size());

            if (auto error = resolve_region(paragraph.region, resolved_paragraph.region)) {
                return {
                    .document = std::nullopt,
                    .error = std::move(error)
                };
            }

            if (resolved_paragraph.region) {
                resolved_paragraph.style = inherited_style(resolved_paragraph.region->style);
            }

            for (const auto& span : paragraph.spans) {
                auto span_timing = resolve_timing(span.timing, resolved_paragraph.timing, mode_);
                if (span_timing.has_error) {
                    continue;
                }

                resolved::Span resolved_span;
                resolved_span.id = span.id;
                resolved_span.timing = std::move(span_timing.timing);
                resolved_span.style = inherited_style(resolved_paragraph.style);
                if (auto error = resolve_region(span.region, resolved_span.region)) {
                    return {
                        .document = {},
                        .error = std::move(error)
                    };
                }

                if (resolved_span.region) {
                    merge_style(resolved_span.style, inherited_style(resolved_span.region->style));
                }
                if (auto error = apply_refs(resolved_span.style, span.style_refs)) {
                    return {
                        .document = {},
                        .error = std::move(error)
                    };
                }
                complete_style(resolved_span.style);
                resolved_span.content = span.content;
                resolved_paragraph.spans.push_back(std::move(resolved_span));
            }

            resolved_division.paragraphs.push_back(std::move(resolved_paragraph));
        }

        output.division = std::move(resolved_division);
        return {
            .document = std::move(output),
            .error = {}
        };
    }

private:
    std::optional<ResolveError> apply_refs(StyleProperties& target, const std::vector<std::string>& refs) {
        for (const auto& ref : refs) {
            StyleProperties resolved;
            if (auto error = resolve_style(ref, resolved)) {
                return error;
            }

            merge_style(target, resolved);
        }
        return {};
    }

    std::optional<ResolveError> resolve_style(std::string_view id, StyleProperties& output) {
        if (const auto cached = resolved_styles_.find(id); cached != resolved_styles_.end()) {
            output = cached->second;
            return {};
        }
        const auto found = styles_.find(id);
        if (found == styles_.end()) {
            return ResolveError{ "unknown style reference: " + std::string{id} };
        }
        if (!resolving_styles_.emplace(id).second) {
            return ResolveError{ "cyclic style reference: " + std::string{id} };
        }

        StyleProperties resolved;
        if (auto error = apply_refs(resolved, found->second->style_refs)) {
            resolving_styles_.erase(id);
            return error;
        }
        merge_style(resolved, found->second->style);
        resolving_styles_.erase(id);
        resolved_styles_.emplace(id, resolved);
        output = std::move(resolved);
        return {};
    }

    std::optional<ResolveError> resolve_region(
        const std::optional<std::string>& id,
        std::optional<resolved::Region>& output) {
        if (!id) return {};
        const auto found = regions_.find(*id);
        if (found == regions_.end()) {
            return ResolveError{ "unknown region reference: " + *id };
        }
        StyleProperties style;
        if (auto error = apply_refs(style, found->second->style_refs)) {
            return error;
        }
        merge_style(style, found->second->style);
        output = resolved::Region{ found->second->id, std::move(style) };
        return {};
    }

    const ast::Document& document_;
    SyncMode mode_;
    std::unordered_map<std::string_view, const ast::StyleDefinition*> styles_;
    std::unordered_map<std::string_view, const ast::RegionDefinition*> regions_;
    std::unordered_map<std::string_view, StyleProperties> resolved_styles_;
    std::unordered_set<std::string_view> resolving_styles_;
};

} // namespace

ResolveResult resolve(const ast::Document& document, SyncMode mode) {
    return Resolver{document, mode}.run();
}

} // namespace ttml

} // namespace arib
