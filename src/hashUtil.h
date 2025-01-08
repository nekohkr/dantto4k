#pragma once
#include <array>
#include <openssl/evp.h>
#include <memory>
#include <vector>
#include <stdexcept>

namespace MmtTlv {

namespace Common {
	using sha256_t = std::array<uint8_t, 0x20>;
	
    inline sha256_t sha256(const std::vector<uint8_t>& input)
    {
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);

        unsigned int digest_len = 0x20;
        EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr);
        EVP_DigestUpdate(ctx.get(), input.data(), input.size());

        sha256_t output;
        EVP_DigestFinal_ex(ctx.get(), output.data(), nullptr);

        return output;
    }
}

}