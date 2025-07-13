#pragma once
#include "smartcard.h"
#include <unordered_map>
#include <map>
#include <array>
#include <optional>
#include <list>
#include "hashUtil.h"

namespace MmtTlv::Acas {

static constexpr uint8_t masterKey[] = {
    0x4F, 0x4C, 0x7C, 0xEB, 0x34, 0xFE, 0xB0, 0xA3,
    0x1E, 0x41, 0x19, 0x51, 0xE1, 0x35, 0x15, 0x12,
    0x87, 0xD3, 0x3D, 0x33, 0xD4, 0x9B, 0x4F, 0x52,
    0x05, 0x77, 0xF9, 0xEF, 0xE5, 0x56, 0x1F, 0x32,
};

struct DecryptedEcm {
    std::array<uint8_t, 16> odd;
    std::array<uint8_t, 16> even;
};

enum class DecryptEcmResult {
    Success,
    CardResetError,
    OtherError
};

class AcasCard {
public:
    AcasCard(SmartCard& smartCard);

    DecryptEcmResult decryptEcm(const std::vector<uint8_t>& ecm, DecryptedEcm& output);
    bool getA0AuthKcl(Common::sha256_t& output);

private:
    SmartCard& smartCard;

};

}