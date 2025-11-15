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
    AESCtrCipher(const std::array<uint8_t, 16>& key)
        : key(key) {
        if (!hasAESNI()) {
            throw std::runtime_error("CPU does not support AES-NI instructions");
        }
        expandKey(key.data());
    }

    static bool hasAESNI() {
        int info[4];
#if defined(_MSC_VER)
        __cpuid(info, 1);
#else
        __asm__ __volatile__("cpuid"
            : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3])
            : "a"(1));
#endif
        return (info[2] & (1 << 25)) != 0;
    }

    void setIv(std::array<uint8_t, 16> iv) {
        this->iv = iv;
    }

    void encrypt(uint8_t* src, size_t size, uint8_t* dst) const {
        __m128i counter = _mm_loadu_si128((const __m128i*)iv.data());
        uint64_t i = 0;

        for (; i + 16 <= size; i += 16) {
            __m128i keyStream = encryptBlock(counter);
            __m128i block = _mm_loadu_si128((const __m128i*)(src + i));
            block = _mm_xor_si128(block, keyStream);
            _mm_storeu_si128((__m128i*)(dst + i), block);

            const __m128i mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
            counter = _mm_shuffle_epi8(counter, mask);
            counter = _mm_add_epi64(counter, _mm_set_epi32(0, 0, 0, 1));
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

    std::array<uint8_t, 16> key;
    std::array<uint8_t, 16> iv;
    __m128i roundKeys[11];
};