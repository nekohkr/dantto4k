#pragma once
#include <cstdint>

namespace MmtTlv {
    
namespace Common {

inline uint16_t swapEndian16(uint16_t num) {
    return (num >> 8) | (num << 8);
}

inline uint32_t swapEndian32(uint32_t num) {
    return ((num >> 24) & 0x000000FF) |
        ((num >> 8) & 0x0000FF00) |
        ((num << 8) & 0x00FF0000) |
        ((num << 24) & 0xFF000000);
}

inline uint64_t swapEndian64(uint64_t num) {
    return ((num >> 56) & 0x00000000000000FFULL) |
        ((num >> 40) & 0x000000000000FF00ULL) |
        ((num >> 24) & 0x0000000000FF0000ULL) |
        ((num >> 8) & 0x00000000FF000000ULL) |
        ((num << 8) & 0x000000FF00000000ULL) |
        ((num << 24) & 0x0000FF0000000000ULL) |
        ((num << 40) & 0x00FF000000000000ULL) |
        ((num << 56) & 0xFF00000000000000ULL);
}
}

}