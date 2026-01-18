#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <immintrin.h>
#include <tmmintrin.h>
#include <stdexcept>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

class AESCtrCipher {
public:
    AESCtrCipher() = default;

    void setKey(const std::array<uint8_t, 16>& newKey) {
        if (!hasAESNI()) {
            throw std::runtime_error("CPU does not support AES-NI instructions");
        }
        key = newKey;
        expandKey(key.data());
    }

    static bool hasAESNI() {
        static bool result = []() {
            int info[4];
#if defined(_MSC_VER)
            __cpuid(info, 1);
#else
            __asm__ __volatile__("cpuid"
                : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3])
                : "a"(1));
#endif
            return (info[2] & (1 << 25)) != 0;
        }();
        return result;
    }

    void setIv(std::array<uint8_t, 16> iv) {
        this->iv = iv;
    }

    void encrypt(uint8_t* src, size_t size, uint8_t* dst) const {
        __m128i counter = _mm_loadu_si128((const __m128i*)iv.data());
        const __m128i mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        const __m128i one_le = _mm_set_epi32(0, 0, 0, 1);
        const __m128i two_le = _mm_set_epi32(0, 0, 0, 2);
        const __m128i three_le = _mm_set_epi32(0, 0, 0, 3);
        const __m128i four_le = _mm_set_epi32(0, 0, 0, 4);

        // BE add constants (add to byte 15)
        const __m128i one_be = _mm_set_epi8(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m128i two_be = _mm_set_epi8(2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m128i three_be = _mm_set_epi8(3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m128i four_be = _mm_set_epi8(4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        uint64_t i = 0;
        int low_byte = iv[15];

        for (; i + 64 <= size; i += 64) {
            __m128i c0 = counter;
            __m128i c1, c2, c3;

            if (low_byte <= 251) {
                // Optimized path: No shuffle needed, use byte addition
                c1 = _mm_add_epi8(c0, one_be);
                c2 = _mm_add_epi8(c0, two_be);
                c3 = _mm_add_epi8(c0, three_be);

                // Update counter for next iteration
                counter = _mm_add_epi8(counter, four_be);
            } else {
                // Fallback path: Shuffle to LE, add, shuffle back
                __m128i c_le = _mm_shuffle_epi8(c0, mask);

                c1 = _mm_shuffle_epi8(_mm_add_epi64(c_le, one_le), mask);
                c2 = _mm_shuffle_epi8(_mm_add_epi64(c_le, two_le), mask);
                c3 = _mm_shuffle_epi8(_mm_add_epi64(c_le, three_le), mask);

                // Update counter for next iteration
                counter = _mm_shuffle_epi8(_mm_add_epi64(c_le, four_le), mask);
            }
            low_byte = (low_byte + 4) & 0xFF;

            encryptBlock4(c0, c1, c2, c3);

            __m128i b0 = _mm_loadu_si128((const __m128i*)(src + i));
            __m128i b1 = _mm_loadu_si128((const __m128i*)(src + i + 16));
            __m128i b2 = _mm_loadu_si128((const __m128i*)(src + i + 32));
            __m128i b3 = _mm_loadu_si128((const __m128i*)(src + i + 48));

            _mm_storeu_si128((__m128i*)(dst + i), _mm_xor_si128(b0, c0));
            _mm_storeu_si128((__m128i*)(dst + i + 16), _mm_xor_si128(b1, c1));
            _mm_storeu_si128((__m128i*)(dst + i + 32), _mm_xor_si128(b2, c2));
            _mm_storeu_si128((__m128i*)(dst + i + 48), _mm_xor_si128(b3, c3));
        }

        for (; i + 16 <= size; i += 16) {
            __m128i keyStream = encryptBlock(counter);
            __m128i block = _mm_loadu_si128((const __m128i*)(src + i));
            block = _mm_xor_si128(block, keyStream);
            _mm_storeu_si128((__m128i*)(dst + i), block);

            counter = _mm_shuffle_epi8(counter, mask);
            counter = _mm_add_epi64(counter, one_le);
            counter = _mm_shuffle_epi8(counter, mask);
        }

        if (i < size)
        {
            uint8_t lastBlock[16] = {};
            memcpy(lastBlock, src + i, size - i);

            __m128i keyStream = encryptBlock(counter);
            __m128i block = _mm_loadu_si128((const __m128i*)(lastBlock));
            block = _mm_xor_si128(block, keyStream);

            _mm_storeu_si128((__m128i*)(lastBlock), block);

            memcpy(dst + i, lastBlock, size - i);
        }
    }

    void decrypt(uint8_t* src, size_t size, uint8_t* dst) const {
        return encrypt(src, size, dst);
    }

private:
    template<uint8_t rcon>
    inline __m128i expandRound(__m128i temp1) const {
        __m128i temp2 = _mm_aeskeygenassist_si128(temp1, rcon);
        __m128i temp3;

        temp2 = _mm_shuffle_epi32(temp2, 0xff);
        temp3 = _mm_slli_si128(temp1, 0x4);
        temp1 = _mm_xor_si128(temp1, temp3);
        temp3 = _mm_slli_si128(temp3, 0x4);
        temp1 = _mm_xor_si128(temp1, temp3);
        temp3 = _mm_slli_si128(temp3, 0x4);
        temp1 = _mm_xor_si128(temp1, temp3);
        temp1 = _mm_xor_si128(temp1, temp2);

        return temp1;
    }

    void expandKey(const uint8_t* key) {
        roundKeys[0] = _mm_loadu_si128((__m128i*)key);
        roundKeys[1] = expandRound<0x01>(roundKeys[0]);
        roundKeys[2] = expandRound<0x02>(roundKeys[1]);
        roundKeys[3] = expandRound<0x04>(roundKeys[2]);
        roundKeys[4] = expandRound<0x08>(roundKeys[3]);
        roundKeys[5] = expandRound<0x10>(roundKeys[4]);
        roundKeys[6] = expandRound<0x20>(roundKeys[5]);
        roundKeys[7] = expandRound<0x40>(roundKeys[6]);
        roundKeys[8] = expandRound<0x80>(roundKeys[7]);
        roundKeys[9] = expandRound<0x1b>(roundKeys[8]);
        roundKeys[10] = expandRound<0x36>(roundKeys[9]);
    }

    __m128i encryptBlock(__m128i counter) const {
        counter = _mm_xor_si128(counter, roundKeys[0]);

        for (int iRound = 1; iRound < 10; ++iRound) {
            counter = _mm_aesenc_si128(counter, roundKeys[iRound]);
        }

        return _mm_aesenclast_si128(counter, roundKeys[10]);
    }

    // Encrypts 4 blocks in parallel using interleaved AES instructions for better pipeline utilization.
    // パイプライン利用効率を向上させるため、インターリーブされたAES命令を使用して4つのブロックを並列に暗号化します。
    void encryptBlock4(__m128i& c0, __m128i& c1, __m128i& c2, __m128i& c3) const {
        __m128i k = roundKeys[0];
        c0 = _mm_xor_si128(c0, k);
        c1 = _mm_xor_si128(c1, k);
        c2 = _mm_xor_si128(c2, k);
        c3 = _mm_xor_si128(c3, k);

        for (int iRound = 1; iRound < 10; ++iRound) {
            k = roundKeys[iRound];
            c0 = _mm_aesenc_si128(c0, k);
            c1 = _mm_aesenc_si128(c1, k);
            c2 = _mm_aesenc_si128(c2, k);
            c3 = _mm_aesenc_si128(c3, k);
        }

        k = roundKeys[10];
        c0 = _mm_aesenclast_si128(c0, k);
        c1 = _mm_aesenclast_si128(c1, k);
        c2 = _mm_aesenclast_si128(c2, k);
        c3 = _mm_aesenclast_si128(c3, k);
    }

    std::array<uint8_t, 16> key{};
    std::array<uint8_t, 16> iv;
    __m128i roundKeys[11];
};
