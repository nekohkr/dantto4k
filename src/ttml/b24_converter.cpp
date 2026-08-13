#include "ttml/b24_converter.h"
#include "aribTextEncoder.h"
#include "b24Color.h"
#include "b24ControlSet.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <utility>
#include <vector>

namespace arib::ttml {

namespace {

struct StyledRun {
    std::string text;
    StyleProperties style;
};

struct Line {
    std::vector<StyledRun> runs;
};

struct RegionBlock {
    resolved::Region region;
    std::vector<Line> lines;
};

struct Page {
    resolved::Timing timing;
    std::vector<RegionBlock> blocks;
};

struct CollectResult {
    std::vector<Page> pages;
    std::optional<B24ConvertError> error;
};

struct TimelineEvent {
    std::chrono::milliseconds time;
    std::vector<const Page*> starts;
    std::vector<const Page*> ends;
};

struct RenderAction {
    std::optional<std::chrono::milliseconds> begin;
    std::uint64_t delay;
    bool clear;
    std::vector<const Page*> pages;
};

bool same_timing(const resolved::Timing& lhs, const resolved::Timing& rhs) {
    return lhs.begin == rhs.begin && lhs.end == rhs.end;
}

std::uint64_t to_deciseconds(std::chrono::milliseconds time) {
    return static_cast<std::uint64_t>(time.count() / 100);
}

std::size_t find_or_add_page(std::vector<Page>& pages, const resolved::Timing& timing) {
    const auto found = std::find_if(pages.begin(), pages.end(),
        [&](const Page& page) { return same_timing(page.timing, timing); });

    if (found != pages.end()) {
        return static_cast<std::size_t>(std::distance(pages.begin(), found));
    }

    pages.push_back(Page{ .timing = timing });
    return pages.size() - 1;
}

std::vector<std::string> split_by_null(std::string_view data) {
    std::vector<std::string> output;
    std::size_t begin = 0;

    for (std::size_t i = 0; i < data.size(); ++i) {
        if (data[i] == '\0') {
            output.emplace_back(data.substr(begin, i - begin));
            begin = i + 1;
        }
    }

    if (begin < data.size()) {
        output.emplace_back(data.substr(begin));
    }
    return output;
}

void append_span(RegionBlock& block, const resolved::Span& span) {
    if (block.lines.empty()) {
        block.lines.emplace_back();
    }

    for (const auto& content : span.content) {
        if (const auto* text = std::get_if<std::string>(&content)) {
            if (!text->empty()) {
                block.lines.back().runs.push_back(StyledRun{
                    .text = *text,
                    .style = span.style,
                });
            }
        }
        else {
            block.lines.emplace_back();
        }
    }
}

class PageBuilder {
public:
    explicit PageBuilder(const resolved::Document& document)
        : document_(document) {
    }

    CollectResult run() const {
        CollectResult output;

        for (const auto& division : document_.divisions) {
            for (const auto& paragraph : division.paragraphs) {
                std::vector<std::optional<std::size_t>> paragraph_block_indexes;

                for (const auto& span : paragraph.spans) {
                    const bool has_text = std::any_of(span.content.begin(), span.content.end(),
                        [](const auto& content) {
                            const auto* text = std::get_if<std::string>(&content);
                            return text && !text->empty();
                        });
                    if (!has_text) {
                        continue;
                    }

                    const resolved::Region* region = span.region
                        ? &*span.region
                        : paragraph.region ? &*paragraph.region : nullptr;

                    if (!region) {
                        output.error = B24ConvertError{ "span has no effective region" };
                        return output;
                    }

                    const auto page_index = find_or_add_page(output.pages, span.timing);
                    Page& page = output.pages[page_index];
                    if (paragraph_block_indexes.size() <= page_index) {
                        paragraph_block_indexes.resize(page_index + 1);
                    }

                    RegionBlock* block;
                    if (span.region) {
                        page.blocks.push_back(RegionBlock{
                            .region = *region,
                            .lines = { Line{} },
                        });
                        block = &page.blocks.back();
                    }
                    else {
                        auto& block_index = paragraph_block_indexes[page_index];
                        if (!block_index) {
                            page.blocks.push_back(RegionBlock{
                                .region = *region,
                                .lines = { Line{} },
                            });
                            block_index = page.blocks.size() - 1;
                        }
                        block = &page.blocks[*block_index];
                    }

                    append_span(*block, span);
                }
            }
        }

        std::stable_sort(output.pages.begin(), output.pages.end(),
            [](const Page& lhs, const Page& rhs) {
                if (!lhs.timing.begin) {
                    return rhs.timing.begin.has_value();
                }
                if (!rhs.timing.begin) {
                    return false;
                }
                return *lhs.timing.begin < *rhs.timing.begin;
            });

        return output;
    }

private:
    const resolved::Document& document_;
};

class TimelineBuilder {
public:
    explicit TimelineBuilder(const std::vector<Page>& pages)
        : pages_(pages) {
    }

    std::vector<TimelineEvent> run() const {
        std::map<std::chrono::milliseconds, TimelineEvent> events;

        for (const auto& page : pages_) {
            const auto begin = page.timing.begin.value_or(std::chrono::milliseconds{});
            auto& start_event = events[begin];
            start_event.time = begin;
            start_event.starts.push_back(&page);

            if (page.timing.end) {
                const auto end = *page.timing.end;
                auto& end_event = events[end];
                end_event.time = end;
                end_event.ends.push_back(&page);
            }
        }

        std::vector<TimelineEvent> output;
        output.reserve(events.size());
        for (auto& entry : events) {
            output.push_back(std::move(entry.second));
        }
        return output;
    }

private:
    const std::vector<Page>& pages_;
};

class Encoder {
public:
    Encoder(const std::vector<Page>& pages, PESType type)
        : pages_(pages), type_(type) {
    }

    B24ConvertResult run() {
        const auto actions = build_render_actions();
        if (type_ == PESType::Sync) {
            if (actions.empty()) {
                return {
                    .outputs = { B24ConvertOutput{
                        .data = {},
                        .begin = std::nullopt,
                        .clear = true,
                    } },
                    .error = std::nullopt,
                };
            }
            return encode_sync(actions);
        }

        append_initial_format();
        for (const auto& action : actions) {
            auto encoded_runs = encode_text_action(action);
            std::size_t encoded_run_index = 0;
            append_time(action.delay);
            if (action.clear) {
                append_clear();
                append_initial_format();
            }

            for (const auto* page : action.pages) {
                encode_page(*page, encoded_runs, encoded_run_index);
            }
        }

        return {
            .outputs = { B24ConvertOutput{
                .data = std::move(output_),
                .begin = pages_.empty() ? std::nullopt : pages_.front().timing.begin,
                .clear = false,
            } },
            .error = std::nullopt,
        };
    }

private:
    B24ConvertResult encode_sync(const std::vector<RenderAction>& actions) {
        B24ConvertResult result;
        for (std::size_t action_index = 0; action_index < actions.size(); ++action_index) {
            const auto& action = actions[action_index];
            const bool is_last_clear = action_index + 1 == actions.size()
                && action.clear
                && action.pages.empty()
                && !result.outputs.empty();
            if (is_last_clear) {
                output_ = std::move(result.outputs.back().data);
                append_time(action.delay);
                append_clear();
                result.outputs.back().data = std::move(output_);
                continue;
            }

            auto encoded_runs = encode_text_action(action);
            std::size_t encoded_run_index = 0;
            output_.clear();
            reset_state();
            append_initial_format();

            for (const auto* page : action.pages) {
                encode_page(*page, encoded_runs, encoded_run_index);
            }

            result.outputs.push_back(B24ConvertOutput{
                .data = std::move(output_),
                .begin = action.begin,
                .clear = action.clear,
            });
        }

        return result;
    }

    std::vector<RenderAction> build_render_actions() const {
        std::vector<RenderAction> actions;
        std::vector<const Page*> active_pages;
        std::chrono::milliseconds current_time{};
        bool first_event = true;

        for (const auto& event : TimelineBuilder{pages_}.run()) {
            const auto delay = first_event && type_ == PESType::Sync
                ? 0
                : to_deciseconds(event.time) - to_deciseconds(current_time);

            for (const auto* page : event.ends) {
                std::erase(active_pages, page);
            }
            for (const auto* page : event.starts) {
                active_pages.push_back(page);
            }

            const bool clear = !event.ends.empty();
            actions.push_back(RenderAction{
                .begin = first_event && !pages_.empty() && !pages_.front().timing.begin
                    ? std::nullopt
                    : std::optional{event.time},
                .delay = delay,
                .clear = clear,
                .pages = type_ == PESType::Sync
                    ? active_pages
                    : (clear ? active_pages : event.starts),
            });

            current_time = event.time;
            first_event = false;
        }
        return actions;
    }

    static void collect_text(const std::vector<const Page*>& pages, std::string& output) {
        for (const auto* page : pages) {
            for (const auto& block : page->blocks) {
                for (const auto& line : block.lines) {
                    for (const auto& run : line.runs) {
                        output.append(run.text);
                        output.push_back('\0');
                    }
                }
            }
        }
    }

    static std::vector<std::string> encode_text_action(const RenderAction& action) {
        std::string text;
        collect_text(action.pages, text);

        return split_by_null(arib::text::encode(text, arib::charset::EncodeMode::Caption));
    }

    void encode_page(const Page& page, const std::vector<std::string>& encoded_runs, std::size_t& encoded_run_index) {
        for (const auto& block : page.blocks) {
            if (block.region.style.extent) {
                append_extent(*block.region.style.extent);
            }

            if (block.region.style.origin) {
                append_origin(*block.region.style.origin);
            }

            append_aps(0, 0);

            for (std::size_t line_index = 0; line_index < block.lines.size(); ++line_index) {
                const auto& line = block.lines[line_index];
                for (const auto& run : line.runs) {
                    if (run.style.letter_spacing) {
                        const auto spacing = static_cast<uint32_t>(std::max(0.0, std::round(*run.style.letter_spacing * 960.0 / 3840.0)));
                        set_character_spacing(spacing);
                    }

                    if (run.style.line_height && run.style.font_size) {
                        const double font_height = run.style.font_size->height;
                        const double line_height = std::holds_alternative<NormalLineHeight>(*run.style.line_height)
                            ? font_height * 1.2
                            : std::get<double>(*run.style.line_height);
                        const auto spacing = static_cast<uint32_t>(std::max(0.0, std::round((line_height - font_height) * 540.0 / 2160.0)));
                        set_line_spacing(spacing);
                    }

                    if (run.style.font_size) {
                        const double width = run.style.font_size->width;
                        const double height = run.style.font_size->height;

                        uint32_t composition_width = static_cast<uint32_t>(std::max(1.0, std::round(width * 960.0 / 3840.0)));
                        uint32_t composition_height = static_cast<uint32_t>(std::max(1.0, std::round(height * 540.0 / 2160.0)));
                        uint8_t size = B24ControlSet::NSZ;
                        set_character_composition(composition_width, composition_height);
                        set_character_size(size);
                    }

                    if (run.style.color) {
                        set_text_color(*run.style.color);
                    }

                    if (run.style.background_color) {
                        set_background_color(*run.style.background_color);
                    }

                    const bool underline = run.style.text_decoration == StyleTextDecorationValue::Underline;
                    set_underline(underline);

                    uint8_t font = 0;
                    if (run.style.font_weight == StyleFontWeightValue::Bold) {
                        font |= 0x01;
                    }
                    if (run.style.font_style == StyleFontStyleValue::Italic) {
                        font |= 0x02;
                    }
                    set_font(font);

                    if (!run.style.text_outline ||
                        std::holds_alternative<None>(*run.style.text_outline)) {
                        set_ornament_off();
                    }
                    else {
                        const auto& text_outline = std::get<TextOutline>(*run.style.text_outline);
                        const auto color = text_outline.color.value_or(
                            run.style.color.value_or(StyleColorValue{ { 255, 255, 255, 255 } }));
                        const auto [palette, index] = findClosestColor(ColorRGBA{
                            color.r(), color.g(), color.b(), color.a()
                        });
                        set_ornament(palette, index);
                    }
                    const auto& encoded = encoded_runs.at(encoded_run_index++);
                    output_.insert(output_.end(), encoded.begin(), encoded.end());
                }
                if (line_index + 1 < block.lines.size()) {
                    output_.push_back(B24ControlSet::APR);
                }
            }
        }
    }


    void reset_state() {
        last_text_color_palette_ = 0;
        last_text_color_index_ = 7;
        last_background_color_palette_ = 0;
        last_background_color_index_ = 8;
        character_size_ = B24ControlSet::NSZ;
        character_composition_width_ = 36;
        character_composition_height_ = 36;
        line_spacing_ = 24;
        character_spacing_ = 4;
        underline_ = false;
        font_ = 0;
        ornament_enabled_ = false;
        ornament_palette_ = 0;
        ornament_index_ = 0;
    }

    void append_clear() {
        output_.push_back(B24ControlSet::CS);
        reset_state();
    }

    void append_initial_format() {
        output_.push_back(B24ControlSet::CSI);
        output_.push_back(0x37);
        output_.push_back(B24ControlSet::SP);
        output_.push_back(B24ControlSet::SWF);
    }

    void append_number(int n) {
        if (n == 0) {
            output_.push_back(0x30);
            return;
        }
        std::vector<uint8_t> temp;
        while (n > 0) {
            temp.push_back(static_cast<uint8_t>((n % 10) + 0x30));
            n /= 10;
        }
        std::reverse(temp.begin(), temp.end());
        output_.insert(output_.end(), temp.begin(), temp.end());
    }

    void append_extent(const StyleExtentValue& extent) {
        output_.push_back(B24ControlSet::CSI);
        append_number(static_cast<uint32_t>(extent.width * 960 / 3840));
        output_.push_back(0x3B);
        append_number(static_cast<uint32_t>(extent.height * 540 / 2160));
        output_.push_back(B24ControlSet::SP);
        output_.push_back(B24ControlSet::SDF);
    }

    void append_origin(const StyleExtentValue& origin) {
        output_.push_back(B24ControlSet::CSI);
        append_number(static_cast<uint32_t>(origin.width * 960 / 3840));
        output_.push_back(0x3B);
        append_number(static_cast<uint32_t>(origin.height * 540 / 2160));
        output_.push_back(B24ControlSet::SP);
        output_.push_back(B24ControlSet::SDP);
    }

    void append_aps(uint8_t row, uint8_t column) {
        output_.push_back(B24ControlSet::APS);
        output_.push_back(static_cast<uint8_t>(0x40 | row));
        output_.push_back(static_cast<uint8_t>(0x40 | column));
    }

    void set_character_composition(uint32_t width, uint32_t height) {
        if (character_composition_width_ == width &&
            character_composition_height_ == height) {
            return;
        }

        output_.push_back(B24ControlSet::CSI);
        append_number(width);
        output_.push_back(0x3B);
        append_number(height);
        output_.push_back(B24ControlSet::SP);
        output_.push_back(B24ControlSet::SSM);

        character_composition_width_ = width;
        character_composition_height_ = height;
    }

    void set_character_size(uint8_t size) {
        if (character_size_ == size) {
            return;
        }

        output_.push_back(size);
        character_size_ = size;
    }

    void append_spacing(uint32_t spacing, uint8_t control) {
        output_.push_back(B24ControlSet::CSI);
        append_number(spacing);
        output_.push_back(B24ControlSet::SP);
        output_.push_back(control);
    }

    void set_line_spacing(uint32_t spacing) {
        if (line_spacing_ == spacing) {
            return;
        }

        append_spacing(spacing, B24ControlSet::SVS);
        line_spacing_ = spacing;
    }

    void set_character_spacing(uint32_t spacing) {
        if (character_spacing_ == spacing) {
            return;
        }

        append_spacing(spacing, B24ControlSet::SHS);
        character_spacing_ = spacing;
    }

    void set_underline(bool underline) {
        if (underline_ == underline) {
            return;
        }

        output_.push_back(underline ? B24ControlSet::STL : B24ControlSet::SPL);
        underline_ = underline;
    }

    void set_text_color(const StyleColorValue& color) {
        const auto [palette, index] = findClosestColor(ColorRGBA{
            color.r(), color.g(), color.b(), color.a()
        });
        if (last_text_color_palette_ == palette && last_text_color_index_ == index) {
            return;
        }

        output_.push_back(B24ControlSet::COL);
        output_.push_back(0x20);
        output_.push_back(static_cast<uint8_t>(0x40 | palette));
        output_.push_back(B24ControlSet::COL);
        output_.push_back(static_cast<uint8_t>(0x40 | index));

        last_text_color_palette_ = palette;
        last_text_color_index_ = index;
    }

    void set_background_color(const StyleColorValue& color) {
        const auto [palette, index] = findClosestColor(ColorRGBA{
            color.r(), color.g(), color.b(), color.a()
        });
        if (last_background_color_palette_ == palette &&
            last_background_color_index_ == index) {
            return;
        }

        output_.push_back(B24ControlSet::COL);
        output_.push_back(0x20);
        output_.push_back(static_cast<uint8_t>(0x40 | palette));
        output_.push_back(B24ControlSet::COL);
        output_.push_back(static_cast<uint8_t>(0x50 | index));

        last_background_color_palette_ = palette;
        last_background_color_index_ = index;
    }

    void append_two_digits(uint8_t value) {
        output_.push_back(static_cast<uint8_t>(0x30 + value / 10));
        output_.push_back(static_cast<uint8_t>(0x30 + value % 10));
    }

    void set_ornament(uint8_t palette, uint8_t index) {
        if (ornament_enabled_ && ornament_palette_ == palette && ornament_index_ == index) {
            return;
        }

        output_.push_back(B24ControlSet::CSI);
        output_.push_back(0x31);
        output_.push_back(0x3B);
        append_two_digits(palette);
        append_two_digits(index);
        output_.push_back(B24ControlSet::SP);
        output_.push_back(B24ControlSet::ORN);

        ornament_enabled_ = true;
        ornament_palette_ = palette;
        ornament_index_ = index;
    }

    void set_ornament_off() {
        if (!ornament_enabled_) {
            return;
        }

        output_.push_back(B24ControlSet::CSI);
        output_.push_back(0x30);
        output_.push_back(B24ControlSet::SP);
        output_.push_back(B24ControlSet::ORN);

        ornament_enabled_ = false;
    }

    void set_font(uint8_t font) {
        if (font_ == font) {
            return;
        }

        output_.push_back(B24ControlSet::CSI);
        output_.push_back(0x30 + font);
        output_.push_back(B24ControlSet::SP);
        output_.push_back(B24ControlSet::MDF);

        font_ = font;
    }

    void append_time(std::uint64_t duration) {
        while (duration > 0) {
            const auto value = static_cast<std::uint8_t>(
                std::min<std::uint64_t>(duration, 0x3F));
            output_.push_back(B24ControlSet::TIME);
            output_.push_back(0x20);
            output_.push_back(0x40 | value);
            duration -= value;
        }
    }

private:
    const std::vector<Page>& pages_;
    PESType type_;
    std::vector<std::uint8_t> output_;
    uint8_t last_text_color_palette_{0};
    uint8_t last_text_color_index_{7};
    uint8_t last_background_color_palette_{0};
    uint8_t last_background_color_index_{8};
    uint8_t character_size_{B24ControlSet::NSZ};
    uint32_t character_composition_width_{36};
    uint32_t character_composition_height_{36};
    uint32_t line_spacing_{24};
    uint32_t character_spacing_{4};
    bool underline_{false};
    uint8_t font_{0};
    bool ornament_enabled_{false};
    uint8_t ornament_palette_{0};
    uint8_t ornament_index_{0};

};

} // namespace

B24ConvertResult convert_to_b24(const resolved::Document& document, PESType type) {
    auto collected = PageBuilder{document}.run();
    if (collected.error) {
        return {
            .outputs = {},
            .error = std::move(collected.error),
        };
    }

    return Encoder{collected.pages, type}.run();
}

} // namespace arib::ttml
