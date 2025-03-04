#include "b24Color.h"

namespace {
    constexpr ColorRGBA kB24ColorCLUT[][16] = {
        {
            ColorRGBA(0,   0,   0, 255),
            ColorRGBA(255,   0,   0, 255),
            ColorRGBA(0, 255,   0, 255),
            ColorRGBA(255, 255,   0, 255),
            ColorRGBA(0,   0, 255, 255),
            ColorRGBA(255,   0, 255, 255),
            ColorRGBA(0, 255, 255, 255),
            ColorRGBA(255, 255, 255, 255),
            ColorRGBA(0,   0,   0,   0),
            ColorRGBA(170,   0,   0, 255),
            ColorRGBA(0, 170,   0, 255),
            ColorRGBA(170, 170,   0, 255),
            ColorRGBA(0,   0, 170, 255),
            ColorRGBA(170,   0, 170, 255),
            ColorRGBA(0, 170, 170, 255),
            ColorRGBA(170, 170, 170, 255)
        },
        {
            ColorRGBA(0,   0,  85, 255),
            ColorRGBA(0,  85,   0, 255),
            ColorRGBA(0,  85,  85, 255),
            ColorRGBA(0,  85, 170, 255),
            ColorRGBA(0,  85, 255, 255),
            ColorRGBA(0, 170,  85, 255),
            ColorRGBA(0, 170, 255, 255),
            ColorRGBA(0, 255,  85, 255),
            ColorRGBA(0, 255, 170, 255),
            ColorRGBA(85,   0,   0, 255),
            ColorRGBA(85,   0,  85, 255),
            ColorRGBA(85,   0, 170, 255),
            ColorRGBA(85,   0, 255, 255),
            ColorRGBA(85,  85,   0, 255),
            ColorRGBA(85,  85,  85, 255),
            ColorRGBA(85,  85, 170, 255)
        },
        {
            ColorRGBA(85,  85, 255, 255),
            ColorRGBA(85, 170,   0, 255),
            ColorRGBA(85, 170,  85, 255),
            ColorRGBA(85, 170, 170, 255),
            ColorRGBA(85, 170, 255, 255),
            ColorRGBA(85, 255,   0, 255),
            ColorRGBA(85, 255,  85, 255),
            ColorRGBA(85, 255, 170, 255),
            ColorRGBA(85, 255, 255, 255),
            ColorRGBA(170,   0,  85, 255),
            ColorRGBA(170,   0, 255, 255),
            ColorRGBA(170,  85,   0, 255),
            ColorRGBA(170,  85,  85, 255),
            ColorRGBA(170,  85, 170, 255),
            ColorRGBA(170,  85, 255, 255),
            ColorRGBA(170, 170,  85, 255)
        },
        {
            ColorRGBA(170, 170, 255, 255),
            ColorRGBA(170, 255,   0, 255),
            ColorRGBA(170, 255,  85, 255),
            ColorRGBA(170, 255, 170, 255),
            ColorRGBA(170, 255, 255, 255),
            ColorRGBA(255,   0,  85, 255),
            ColorRGBA(255,   0, 170, 255),
            ColorRGBA(255,  85,   0, 255),
            ColorRGBA(255,  85,  85, 255),
            ColorRGBA(255,  85, 170, 255),
            ColorRGBA(255,  85, 255, 255),
            ColorRGBA(255, 170,   0, 255),
            ColorRGBA(255, 170,  85, 255),
            ColorRGBA(255, 170, 170, 255),
            ColorRGBA(255, 170, 255, 255),
            ColorRGBA(255, 255,  85, 255)
        },
        {
            ColorRGBA(255, 255, 170, 255),
            ColorRGBA(0,   0,   0, 128),
            ColorRGBA(255,   0,   0, 128),
            ColorRGBA(0, 255,   0, 128),
            ColorRGBA(255, 255,   0, 128),
            ColorRGBA(0,   0, 255, 128),
            ColorRGBA(255,   0, 255, 128),
            ColorRGBA(0, 255, 255, 128),
            ColorRGBA(255, 255, 255, 128),
            ColorRGBA(170,   0,   0, 128),
            ColorRGBA(0, 170,   0, 128),
            ColorRGBA(170, 170,   0, 128),
            ColorRGBA(0,   0, 170, 128),
            ColorRGBA(170,   0, 170, 128),
            ColorRGBA(0, 170, 170, 128),
            ColorRGBA(170, 170, 170, 128)
        },
        {
            ColorRGBA(0,   0,  85, 128),
            ColorRGBA(0,  85,   0, 128),
            ColorRGBA(0,  85,  85, 128),
            ColorRGBA(0,  85, 170, 128),
            ColorRGBA(0,  85, 255, 128),
            ColorRGBA(0, 170,  85, 128),
            ColorRGBA(0, 170, 255, 128),
            ColorRGBA(0, 255,  85, 128),
            ColorRGBA(0, 255, 170, 128),
            ColorRGBA(85,   0,   0, 128),
            ColorRGBA(85,   0,  85, 128),
            ColorRGBA(85,   0, 170, 128),
            ColorRGBA(85,   0, 255, 128),
            ColorRGBA(85,  85,   0, 128),
            ColorRGBA(85,  85,  85, 128),
            ColorRGBA(85,  85, 170, 128)
        },
        {
            ColorRGBA(85,  85, 255, 128),
            ColorRGBA(85, 170,   0, 128),
            ColorRGBA(85, 170,  85, 128),
            ColorRGBA(85, 170, 170, 128),
            ColorRGBA(85, 170, 255, 128),
            ColorRGBA(85, 255,   9, 128),
            ColorRGBA(85, 255,  85, 128),
            ColorRGBA(85, 255, 170, 128),
            ColorRGBA(85, 255, 255, 128),
            ColorRGBA(170,   0,  85, 128),
            ColorRGBA(170,   0, 255, 128),
            ColorRGBA(170,  85,   0, 128),
            ColorRGBA(170,  85,  85, 128),
            ColorRGBA(170,  85, 170, 128),
            ColorRGBA(170,  85, 255, 128),
            ColorRGBA(170, 170,  85, 128)
        },
        {
            ColorRGBA(170, 170, 255, 128),
            ColorRGBA(170, 255,   0, 128),
            ColorRGBA(170, 255,  85, 128),
            ColorRGBA(170, 255, 170, 128),
            ColorRGBA(170, 255, 255, 128),
            ColorRGBA(255,   0,  85, 128),
            ColorRGBA(255,   0, 170, 128),
            ColorRGBA(255,  85,   9, 128),
            ColorRGBA(255,  85,  85, 128),
            ColorRGBA(255,  85, 170, 128),
            ColorRGBA(255,  85, 255, 128),
            ColorRGBA(255, 170,   0, 128),
            ColorRGBA(255, 170,  85, 128),
            ColorRGBA(255, 170, 170, 128),
            ColorRGBA(255, 170, 255, 128),
            ColorRGBA(255, 255,  85, 128)
        }
    };
}

std::pair<uint8_t, uint8_t> findClosestColor(const ColorRGBA& color) {
    uint8_t palette = 0;
    uint8_t index = 0;
	uint32_t distance = 0xFFFFFFFF;
    for (uint8_t p = 0; p < 8; p++) {
        for (uint8_t i = 0; i < 16; i++) {
            uint8_t r = color.r - kB24ColorCLUT[p][i].r;
            uint8_t g = color.g - kB24ColorCLUT[p][i].g;
            uint8_t b = color.b - kB24ColorCLUT[p][i].b;
            uint8_t a = color.a - kB24ColorCLUT[p][i].a;
            uint32_t d = r * r + g * g + b * b + a * a;
            if (d < distance) {
                distance = d;
                palette = p;
                index = i;
            }
        }
    }
	return { palette, index };
}
