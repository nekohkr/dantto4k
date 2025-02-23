#pragma once
#include <cstdint>
#include <utility>

struct ColorRGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
    constexpr ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}
};

std::pair<uint8_t, uint8_t> findClosestColor(const ColorRGBA& color);
std::pair<uint8_t, uint8_t> findClosestFontColor(const ColorRGBA& color);

