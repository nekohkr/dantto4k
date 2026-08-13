#include "ttml.h"
#include "pugixml.hpp"

namespace {

uint64_t parseTimestamp(const std::string& timestamp) {
    size_t dotPos = timestamp.find('.');
    uint64_t hours, minutes, seconds, millis = 0;

    if (dotPos != std::string::npos) {
        if (timestamp.size() < 10) {
            throw std::invalid_argument("Invalid timestamp format: " + timestamp);
        }
        if (timestamp[2] != ':' || timestamp[5] != ':') {
            throw std::invalid_argument("Invalid timestamp format: " + timestamp);
        }
        hours = std::stoull(timestamp.substr(0, 2));
        minutes = std::stoull(timestamp.substr(3, 2));
        seconds = std::stoull(timestamp.substr(6, 2));
        std::string frac = timestamp.substr(dotPos + 1);
        if (frac.empty() || frac.size() > 3) {
            throw std::invalid_argument("Invalid fractional seconds in timestamp: " + timestamp);
        }
        if (frac.size() == 1) {
            millis = std::stoull(frac) * 100;
        }
        else if (frac.size() == 2) {
            millis = std::stoull(frac) * 10;
        }
        else {
            millis = std::stoull(frac);
        }
    }
    else {
        if (timestamp.size() != 8 || timestamp[2] != ':' || timestamp[5] != ':') {
            throw std::invalid_argument("Invalid timestamp format: " + timestamp);
        }
        hours = std::stoull(timestamp.substr(0, 2));
        minutes = std::stoull(timestamp.substr(3, 2));
        seconds = std::stoull(timestamp.substr(6, 2));
    }

    return hours * 3600 * 1000ULL + minutes * 60 * 1000ULL + seconds * 1000ULL + millis;
}

void mergeStyle(TTMLStyle& target, const TTMLStyle& source) {
    if (source.fontSize) target.fontSize = source.fontSize;
    if (source.lineHeight) target.lineHeight = source.lineHeight;
    if (source.letterSpacing) target.letterSpacing = source.letterSpacing;
    if (source.fontWeight) target.fontWeight = source.fontWeight;
    if (source.fontStyle) target.fontStyle = source.fontStyle;
    if (source.color) target.color = source.color;
    if (source.backgroundColor) target.backgroundColor = source.backgroundColor;
    if (source.outlineSpecified) {
        target.outlineSpecified = true;
        target.outlineColor = source.outlineColor;
    }
    if (source.fontFamily) target.fontFamily = source.fontFamily;
    if (source.textDecoration) target.textDecoration = source.textDecoration;
    if (source.writingMode) target.writingMode = source.writingMode;
    if (source.opacity) target.opacity = source.opacity;
    if (source.border) target.border = source.border;
    if (source.animation) target.animation = source.animation;
    if (source.ruby) target.ruby = source.ruby;
    if (source.textShadow) target.textShadow = source.textShadow;
    if (source.overflow) target.overflow = source.overflow;
}

TTMLStyle inheritedStyle(const TTMLStyle& parent) {
    TTMLStyle result;
    result.fontSize = parent.fontSize;
    result.lineHeight = parent.lineHeight;
    result.letterSpacing = parent.letterSpacing;
    result.fontWeight = parent.fontWeight;
    result.fontStyle = parent.fontStyle;
    result.color = parent.color;
    result.outlineSpecified = parent.outlineSpecified;
    result.outlineColor = parent.outlineColor;
    result.fontFamily = parent.fontFamily;
    result.textDecoration = parent.textDecoration;
    result.writingMode = parent.writingMode;
    result.ruby = parent.ruby;
    result.textShadow = parent.textShadow;
    // tts:backgroundColor is not inherited in TTML.
    return result;
}

TTMLStyle initialStyle() {
    TTMLStyle result;
    // TR-B39 has no initial fontSize.  Keep it absent so callers can
    // distinguish an invalid/incomplete document from a 144px style.
    result.lineHeight = TtmlStyleValue(TtmlStyleValueKeyword("normal"));
    result.letterSpacing = TtmlStyleValue(TtmlStyleValueLength(0.0f, "px"));
    result.fontWeight = TtmlStyleValue(TtmlStyleValueKeyword("normal"));
    result.fontStyle = TtmlStyleValue(TtmlStyleValueKeyword("normal"));
    result.color = TtmlStyleValue(TtmlStyleValueColor(255, 255, 255, 255));
    result.backgroundColor = TtmlStyleValue(TtmlStyleValueColor(0, 0, 0, 0));
    result.outlineSpecified = true;
    result.fontFamily = "round gothic";
    result.textDecoration = TtmlStyleValue(TtmlStyleValueKeyword("none"));
    result.writingMode = TtmlStyleValue(TtmlStyleValueKeyword("lrtb"));
    result.opacity = TtmlStyleValue(TtmlStyleValueNumber(1.0f));
    result.border = "none 0px #00000000";
    result.animation = TtmlStyleValue(TtmlStyleValueKeyword("none"));
    result.ruby = TtmlStyleValue(TtmlStyleValueKeyword("none"));
    result.textShadow = "none";
    result.overflow = TtmlStyleValue(TtmlStyleValueKeyword("hidden"));
    // textOutline initial value is "none", represented by no color.
    return result;
}

void completeComputedStyle(TTMLStyle& style) {
    const TTMLStyle defaults = initialStyle();
    if (!style.lineHeight) style.lineHeight = defaults.lineHeight;
    if (!style.letterSpacing) style.letterSpacing = defaults.letterSpacing;
    if (!style.fontWeight) style.fontWeight = defaults.fontWeight;
    if (!style.fontStyle) style.fontStyle = defaults.fontStyle;
    if (!style.color) style.color = defaults.color;
    if (!style.backgroundColor) style.backgroundColor = defaults.backgroundColor;
    if (!style.fontFamily) style.fontFamily = defaults.fontFamily;
    if (!style.textDecoration) style.textDecoration = defaults.textDecoration;
    if (!style.writingMode) style.writingMode = defaults.writingMode;
    if (!style.opacity) style.opacity = defaults.opacity;
    if (!style.border) style.border = defaults.border;
    if (!style.animation) style.animation = defaults.animation;
    if (!style.ruby) style.ruby = defaults.ruby;
    if (!style.textShadow) style.textShadow = defaults.textShadow;
    if (!style.overflow) style.overflow = defaults.overflow;
}

void applyTextOutline(const pugi::xml_attribute& attribute, TTMLStyle& target) {
    if (!attribute) {
        return;
    }

    target.outlineSpecified = true;
    target.outlineColor.reset();
    std::istringstream valueStream(attribute.value());
    std::string color;
    valueStream >> color;
    if (color == "none") {
        return;
    }
    if (!color.empty() && color.front() == '#') {
        target.outlineColor = TtmlStyleValueParser::parse(color);
    }
}

void applyReferencedStyles(const pugi::xml_node& node, const std::list<TTMLStyle>& styles, TTMLStyle& target) {
    std::istringstream styleStream(node.attribute("style").value());
    std::string styleId;
    while (styleStream >> styleId) {
        const auto style = std::find_if(styles.begin(), styles.end(), [&styleId](const TTMLStyle& candidate) {
            return candidate.id == styleId;
        });
        if (style != styles.end()) {
            mergeStyle(target, *style);
        }
    }
}

void applyInlineStyles(const pugi::xml_node& node, TTMLStyle& target) {
    if (node.attribute("tts:fontSize")) {
        target.fontSize = TtmlStyleValueParser::parseFontSize(node.attribute("tts:fontSize").value());
    }
    if (node.attribute("tts:lineHeight")) {
        target.lineHeight = TtmlStyleValueParser::parse(node.attribute("tts:lineHeight").value());
    }
    if (node.attribute("arib-tt:letter-spacing")) {
        target.letterSpacing = TtmlStyleValueParser::parse(node.attribute("arib-tt:letter-spacing").value());
    }
    if (node.attribute("tts:fontWeight")) {
        target.fontWeight = TtmlStyleValueParser::parse(node.attribute("tts:fontWeight").value());
    }
    if (node.attribute("tts:fontStyle")) {
        target.fontStyle = TtmlStyleValueParser::parse(node.attribute("tts:fontStyle").value());
    }
    if (node.attribute("tts:color")) {
        target.color = TtmlStyleValueParser::parse(node.attribute("tts:color").value());
    }
    if (node.attribute("tts:backgroundColor")) {
        target.backgroundColor = TtmlStyleValueParser::parse(node.attribute("tts:backgroundColor").value());
    }
    if (node.attribute("tts:fontFamily")) {
        target.fontFamily = node.attribute("tts:fontFamily").value();
    }
    if (node.attribute("tts:textDecoration")) {
        target.textDecoration = TtmlStyleValueParser::parse(node.attribute("tts:textDecoration").value());
    }
    if (node.attribute("tts:writingMode")) {
        target.writingMode = TtmlStyleValueParser::parse(node.attribute("tts:writingMode").value());
    }
    if (node.attribute("tts:opacity")) {
        target.opacity = TtmlStyleValueParser::parse(node.attribute("tts:opacity").value());
    }
    if (node.attribute("arib-tt:border")) {
        target.border = node.attribute("arib-tt:border").value();
    }
    if (node.attribute("arib-tt:animation")) {
        target.animation = TtmlStyleValueParser::parse(node.attribute("arib-tt:animation").value());
    }
    if (node.attribute("arib-tt:ruby")) {
        target.ruby = TtmlStyleValueParser::parse(node.attribute("arib-tt:ruby").value());
    }
    if (node.attribute("arib-tt:text-shadow")) {
        target.textShadow = node.attribute("arib-tt:text-shadow").value();
    }
    if (node.attribute("tts:overflow")) {
        target.overflow = TtmlStyleValueParser::parse(node.attribute("tts:overflow").value());
    }
    applyTextOutline(node.attribute("tts:textOutline"), target);
}

}

TTML TTMLPaser::parse(const std::string& input) {
    TTML output;
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(input.data(), input.size());
    if (result.status != pugi::status_ok) {
        return {};
    }

    for (pugi::xml_node p : doc.child("tt").child("head").child("layout").children("region")) {
        TTMLRegion region;
        region.id = p.attribute("xml:id").value();
        if (p.attribute("tts:extent")) {
            const auto [width, height] = TtmlStyleValueParser::parseLengthPair(p.attribute("tts:extent").value());
            region.extent = TtmlStyleExtent{ width, height };
        }
        if (p.attribute("tts:origin")) {
            const auto [x, y] = TtmlStyleValueParser::parseLengthPair(p.attribute("tts:origin").value());
            region.origin = TtmlStyleOrigin{ x, y };
        }
        output.regions.push_back(region);
    }

    for (pugi::xml_node p : doc.child("tt").child("head").child("styling").children("style")) {
        TTMLStyle style;
        style.id = p.attribute("xml:id").value();
        applyInlineStyles(p, style);

        output.styles.push_back(style);
    }

    // A style element may itself reference one or more styles.  Resolve those
    // references before content styles are computed; the element's own values
    // have higher priority than its referenced styles.
    for (pugi::xml_node node : doc.child("tt").child("head").child("styling").children("style")) {
        const std::string id = node.attribute("xml:id").value();
        auto own = std::find_if(output.styles.begin(), output.styles.end(), [&id](const TTMLStyle& style) {
            return style.id == id;
        });
        if (own == output.styles.end() || !node.attribute("style")) {
            continue;
        }
        TTMLStyle resolved;
        applyReferencedStyles(node, output.styles, resolved);
        mergeStyle(resolved, *own);
        resolved.id = own->id;
        *own = resolved;
    }

    const pugi::xml_node body = doc.child("tt").child("body");
    TTMLStyle bodyStyle = initialStyle();
    applyReferencedStyles(body, output.styles, bodyStyle);
    applyInlineStyles(body, bodyStyle);

    for (pugi::xml_node div : body.children("div")) {
        TTMLDivTag divTag;
        TTMLStyle divStyle = inheritedStyle(bodyStyle);
        applyReferencedStyles(div, output.styles, divStyle);
        applyInlineStyles(div, divStyle);
        if (div.attribute("begin")) {
            divTag.begin = parseTimestamp(div.attribute("begin").value());
        }
        if (div.attribute("end")) {
            divTag.end = parseTimestamp(div.attribute("end").value());
        }

        for (pugi::xml_node p : div.children("p")) {
            std::string regionId = p.attribute("region").value();
            auto region = std::find_if(output.regions.begin(), output.regions.end(), [regionId](const TTMLRegion& r) {
                return r.id == regionId;
                });

            TTMLPTag pTag;
            TTMLStyle pStyle = inheritedStyle(divStyle);
            applyReferencedStyles(p, output.styles, pStyle);
            applyInlineStyles(p, pStyle);
            pTag.id = p.attribute("xml:id").value();
            if (region != output.regions.end()) {
                pTag.region = *region;
            }

            for (pugi::xml_node span : p.children("span")) {
                TTMLSpanTag spanTag;
                spanTag.id = span.attribute("xml:id").value();
                spanTag.text = span.text().get();
                spanTag.style = inheritedStyle(pStyle);
                applyReferencedStyles(span, output.styles, spanTag.style);
                applyInlineStyles(span, spanTag.style);
                completeComputedStyle(spanTag.style);

                pTag.spanTags.push_back(spanTag);
            }

            if (pTag.spanTags.empty()) {
                const std::string text = p.text().get();
                if (!text.empty()) {
                    TTMLSpanTag spanTag;
                    spanTag.text = text;
                    spanTag.style = inheritedStyle(pStyle);
                    completeComputedStyle(spanTag.style);
                    pTag.spanTags.push_back(std::move(spanTag));
                }
            }

            divTag.pTags.push_back(pTag);
        }

        output.divTags.push_back(divTag);
    }

	return output;
}

TtmlStyleValueColor TtmlStyleValueColor::parse(std::string_view text) {
    // ARIB TR-B39 8.5.6.4 operates only #RRGGBB and #RRGGBBAA.
    if ((text.size() != 7 && text.size() != 9) || text.front() != '#') {
        throw std::invalid_argument("Invalid ARIB-TTML color. Expected #RRGGBB or #RRGGBBAA");
    }

    const unsigned long value = std::stoul(std::string(text.substr(1)), nullptr, 16);
    const uint8_t alpha = text.size() == 7 ? 0xFF : static_cast<uint8_t>(value & 0xFF);
    const uint8_t blue = text.size() == 7 ? static_cast<uint8_t>(value & 0xFF) : static_cast<uint8_t>((value >> 8) & 0xFF);
    const uint8_t green = text.size() == 7 ? static_cast<uint8_t>((value >> 8) & 0xFF) : static_cast<uint8_t>((value >> 16) & 0xFF);
    const uint8_t red = text.size() == 7 ? static_cast<uint8_t>((value >> 16) & 0xFF) : static_cast<uint8_t>((value >> 24) & 0xFF);
    return { red, green, blue, alpha };
}

TtmlStyleValue TtmlStyleValueParser::parse(const std::string& input) {
    try {
        if (input.find("px") != std::string::npos || input.find("em") != std::string::npos ||
            input.find("rem") != std::string::npos || input.find("%") != std::string::npos) {
            std::regex regex(R"(^([+-]?\d*\.?\d+)(px|em|rem|%)$)");
            std::smatch match;
            if (std::regex_match(input, match, regex)) {
                return TtmlStyleValue(TtmlStyleValueLength(std::stof(match[1]), match[2]));
            }
            else {
                throw std::invalid_argument("Invalid length value: " + input);
            }
        }

        if (input.find("#") == 0) {
            return TtmlStyleValue(TtmlStyleValueColor::parse(input));
        }

        static const std::set<std::string> validKeywords = {
            "bold", "italic", "normal", "none", "underline", "lrtb", "tbrl",
            "hidden", "visible", "ruby", "emphasis", "base", "text", "container"
        };
        if (validKeywords.find(input) != validKeywords.end()) {
            return TtmlStyleValue(TtmlStyleValueKeyword(input));
        }

        return TtmlStyleValue(TtmlStyleValueNumber(std::stof(input)));

    }
    catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

std::pair<TtmlStyleValueLength, TtmlStyleValueLength> TtmlStyleValueParser::parseLengthPair(const std::string& input) {
    std::istringstream iss(input);
    std::string token1, token2;
    if (!(iss >> token1)) {
        throw std::invalid_argument("Failed to parse TTML value pair from: " + input);
    }
    if (!(iss >> token2)) {
        token2 = token1;
    }
    const TtmlStyleValue value1 = parse(token1);
    const TtmlStyleValue value2 = parse(token2);
    if (!value1.isValue<TtmlStyleValueLength>() || !value2.isValue<TtmlStyleValueLength>()) {
        throw std::invalid_argument("Expected TTML length pair: " + input);
    }
    return std::make_pair(value1.getValue<TtmlStyleValueLength>(), value2.getValue<TtmlStyleValueLength>());
}

TtmlStyleFontSize TtmlStyleValueParser::parseFontSize(const std::string& input) {
    const auto [width, height] = parseLengthPair(input);
    return { width, height };
}
