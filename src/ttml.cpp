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

}

TTML TTMLPaser::parse(const std::vector<uint8_t>& input) {
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
            region.extent = TTMLCssValueParser::parsePair(p.attribute("tts:extent").value());
        }
        if (p.attribute("tts:origin")) {
            region.origin = TTMLCssValueParser::parsePair(p.attribute("tts:origin").value());
        }
        output.regions.push_back(region);
    }

    for (pugi::xml_node p : doc.child("tt").child("head").child("styling").children("style")) {
        TTMLStyle style;
        style.id = p.attribute("xml:id").value();
        if (p.attribute("tts:fontSize")) {
            style.fontSize = TTMLCssValueParser::parsePair(p.attribute("tts:fontSize").value());
        }
        if (p.attribute("tts:lineHeight")) {
            style.lineHeight = TTMLCssValueParser::parse(p.attribute("tts:lineHeight").value());
        }
        if (p.attribute("tts:fontWeight")) {
            style.fontWeight = TTMLCssValueParser::parse(p.attribute("tts:fontWeight").value());
        }
        if (p.attribute("tts:fontStyle")) {
            style.fontStyle = TTMLCssValueParser::parse(p.attribute("tts:fontStyle").value());
        }
        if (p.attribute("tts:color")) {
            style.color = TTMLCssValueParser::parse(p.attribute("tts:color").value());
        }
        if (p.attribute("tts:backgroundColor")) {
            style.backgroundColor = TTMLCssValueParser::parse(p.attribute("tts:backgroundColor").value());
        }

        output.styles.push_back(style);
    }

    for (pugi::xml_node div : doc.child("tt").child("body").children("div")) {
        if (!div.attribute("begin")) {
            continue;
        }

        TTMLDivTag divTag;
        divTag.begin = parseTimestamp(div.attribute("begin").value());
        if (div.attribute("end")) {
            divTag.end = parseTimestamp(div.attribute("end").value());
        }

        for (pugi::xml_node p : div.children("p")) {
            std::string regionId = p.attribute("region").value();
            auto region = std::find_if(output.regions.begin(), output.regions.end(), [regionId](const TTMLRegion& r) {
                return r.id == regionId;
                });

            TTMLPTag pTag;
            pTag.id = p.attribute("xml:id").value();
            if (region != output.regions.end()) {
                pTag.region = *region;
            }

            for (pugi::xml_node span : p.children("span")) {
                TTMLSpanTag spanTag;
                spanTag.id = span.attribute("xml:id").value();
                spanTag.text = span.text().get();

                std::string styleStr = span.attribute("style").value();
                std::istringstream styleStream(styleStr);
                std::string styleId;
                while (styleStream >> styleId) {
                    auto style = std::find_if(output.styles.begin(), output.styles.end(), [styleId](const TTMLStyle& s) {
                        return s.id == styleId;
                        });
                    if (style != output.styles.end()) {
                        if (style->fontSize.has_value()) {
                            spanTag.style.fontSize = style->fontSize;
                        }
                        if (style->lineHeight.has_value()) {
                            spanTag.style.lineHeight = style->lineHeight;
                        }
                        if (style->fontWeight.has_value()) {
                            spanTag.style.fontWeight = style->fontWeight;
                        }
                        if (style->fontStyle.has_value()) {
                            spanTag.style.fontStyle = style->fontStyle;
                        }
                        if (style->color.has_value()) {
                            spanTag.style.color = style->color;
                        }
                        if (style->backgroundColor.has_value()) {
                            spanTag.style.backgroundColor = style->backgroundColor;
                        }
                    }
                }

                pTag.spanTags.push_back(spanTag);
            }

            divTag.pTags.push_back(pTag);
        }

        output.divTags.push_back(divTag);
    }

	return output;
}
