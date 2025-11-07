#pragma once
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using sha256_t = std::array<uint8_t, 32>;

class SHA256 {
public:
    SHA256() { reset(); }

    void update(std::string_view data) {
        update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    void update(const std::vector<uint8_t>& data) {
        update(data.data(), data.size());
    }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            buffer_[buffer_size_++] = data[i];
            if (buffer_size_ == 64) {
                transform();
                bit_length_ += 512;
                buffer_size_ = 0;
            }
        }
    }

    sha256_t finalize() {
        sha256_t hash;
        pad();
        revert(hash);
        reset();
        return hash;
    }

    static sha256_t hash(std::string_view data) {
        SHA256 sha;
        sha.update(data);
        return sha.finalize();
    }

    static sha256_t hash(const std::vector<uint8_t>& data) {
        SHA256 sha;
        sha.update(data);
        return sha.finalize();
    }

    static std::string toString(const sha256_t& hash) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (uint8_t byte : hash) {
            oss << std::setw(2) << static_cast<int>(byte);
        }
        return oss.str();
    }

private:
    static constexpr std::array<uint32_t, 64> K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    std::array<uint32_t, 8> state_;
    std::array<uint8_t, 64> buffer_;
    size_t buffer_size_;
    uint64_t bit_length_;

    void reset() {
        state_ = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        buffer_size_ = 0;
        bit_length_ = 0;
    }

    void transform() {
        std::array<uint32_t, 64> m;

        for (size_t i = 0, j = 0; i < 16; ++i, j += 4) {
            m[i] = (buffer_[j] << 24) | (buffer_[j + 1] << 16) |
                (buffer_[j + 2] << 8) | (buffer_[j + 3]);
        }

        for (size_t i = 16; i < 64; ++i) {
            m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];
        }

        auto [a, b, c, d, e, f, g, h] = state_;
        for (size_t i = 0; i < 64; ++i) {
            uint32_t temp1 = h + ep1(e) + ch(e, f, g) + K[i] + m[i];
            uint32_t temp2 = ep0(a) + maj(a, b, c);

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    void pad() {
        uint64_t i = buffer_size_;
        uint8_t end = buffer_size_ < 56 ? 56 : 64;

        buffer_[i++] = 0x80;
        while (i < end) {
            buffer_[i++] = 0x00;
        }

        if (buffer_size_ >= 56) {
            transform();
            std::fill(buffer_.begin(), buffer_.begin() + 56, 0);
        }

        bit_length_ += buffer_size_ * 8;
        buffer_[63] = static_cast<uint8_t>(bit_length_);
        buffer_[62] = static_cast<uint8_t>(bit_length_ >> 8);
        buffer_[61] = static_cast<uint8_t>(bit_length_ >> 16);
        buffer_[60] = static_cast<uint8_t>(bit_length_ >> 24);
        buffer_[59] = static_cast<uint8_t>(bit_length_ >> 32);
        buffer_[58] = static_cast<uint8_t>(bit_length_ >> 40);
        buffer_[57] = static_cast<uint8_t>(bit_length_ >> 48);
        buffer_[56] = static_cast<uint8_t>(bit_length_ >> 56);
        transform();
    }

    void revert(sha256_t& hash) {
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                hash[i + (j * 4)] = (state_[j] >> (24 - i * 8)) & 0xff;
            }
        }
    }

    static constexpr uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }

    static constexpr uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }

    static constexpr uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    static constexpr uint32_t ep0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }

    static constexpr uint32_t ep1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }

    static constexpr uint32_t sig0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    static constexpr uint32_t sig1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }
};