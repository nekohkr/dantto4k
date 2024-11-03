#pragma once
#include "smartcard.h"
#include <unordered_map>
#include <map>

namespace MmtTlv::Acas {

struct DecryptedEcm {
public:
    uint8_t odd[16];
    uint8_t even[16];
};

class AcasCard {
public:
    AcasCard(std::shared_ptr<SmartCard> smartCard);
    void init();
    std::vector<uint8_t> getA0AuthKcl();
    DecryptedEcm decryptEcm(std::vector<uint8_t>& ecm);
    DecryptedEcm lastDecryptedEcm;
    bool ready = false;
    void clear();

private:
    static constexpr uint8_t masterKey[] =
    {
            0x4F, 0x4C, 0x7C, 0xEB, 0x34, 0xFE, 0xB0, 0xA3,
                0x1E, 0x41, 0x19, 0x51, 0xE1, 0x35, 0x15, 0x12,
                0x87, 0xD3, 0x3D, 0x33, 0xD4, 0x9B, 0x4F, 0x52,
                0x05, 0x77, 0xF9, 0xEF, 0xE5, 0x56, 0x1F, 0x32,
    };

    std::map<std::vector<uint8_t>, DecryptedEcm> decryptedEcmMap;
    std::shared_ptr<SmartCard> smartCard;
};

}