#include "acasCard.h"
#include <random>
#include <algorithm>
#include "config.h"

uint32_t AcasCard::getA0AuthKcl(sha256_t& output) {
    std::default_random_engine engine(std::random_device{}());
    std::uniform_int_distribution<int> distrib(0, 255);

    std::vector<uint8_t> data = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x8A, 0xF7 };
    std::vector<uint8_t> a0init(8);

    for (size_t i = 0; i < 8; ++i) {
        a0init[i] = static_cast<uint8_t>(distrib(engine));
    }

    data.insert(data.end(), a0init.begin(), a0init.end());

    ApduCommand apdu(0x90, 0xA0, 0x00, 0x01);
    ApduResponse response;

    if (!smartCard->isInited()) {
        smartCard->init();
    }
    if (!smartCard->isConnected()) {
        smartCard->connect();
    }

    LONG result = smartCard->transmit(apdu.case4short(data, 0x00), response);
    if (result != SCARD_S_SUCCESS) {
        return result;
    }

    if (!response.isSuccess()) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto a0data = response.getData();
    std::vector<uint8_t> a0response(a0data.begin() + 0x06, a0data.begin() + 0x06 + 0x08);
    std::vector<uint8_t> a0hash(a0data.begin() + 0x0e, a0data.end());

    std::vector<uint8_t> plainKcl;
    plainKcl.insert(plainKcl.end(), std::begin(masterKey), std::end(masterKey));
    plainKcl.insert(plainKcl.end(), a0init.begin(), a0init.end());
    plainKcl.insert(plainKcl.end(), a0response.begin(), a0response.end());

    sha256_t kcl = SHA256::hash(plainKcl);

    std::vector<uint8_t> plainData;
    plainData.insert(plainData.end(), kcl.begin(), kcl.end());
    plainData.insert(plainData.end(), a0init.begin(), a0init.end());

    sha256_t hash = SHA256::hash(plainData);

    if (!std::equal(hash.begin(), hash.end(), a0hash.begin())) {
        return SCARD_F_INTERNAL_ERROR;
    }

    output = kcl;

    return SCARD_S_SUCCESS;
}

bool AcasCard::ecm(const std::vector<uint8_t>& ecm, DecryptionKey& output) {
    ApduResponse response;
    sha256_t kcl;
    uint32_t retryCount = 0;
    uint32_t retryReconnectCount = 0;
    if (smartCard == nullptr) {
        return false;
    }

    try {
    retry:
        if (cardReset) {
        retryReconnect:
            if (retryReconnectCount > 4) {
                return false;
            }

            uint32_t result = smartCard->reconnect();
            if (result != SCARD_S_SUCCESS) {
                ++retryReconnectCount;
                goto retryReconnect;
            }
            cardReset = false;
        }

        if (!smartCard->isInited()) {
            smartCard->init();
        }
        if (!smartCard->isConnected()) {
            smartCard->connect();
        }

        {
            auto scope = smartCard->scopedTransaction();

            uint32_t result = getA0AuthKcl(kcl);
            if (result != SCARD_S_SUCCESS) {
                if (result == SCARD_W_RESET_CARD) {
                    if (retryCount > 4) {
                        return false;
                    }

                    cardReset = true;
                    ++retryCount;
                    goto retry;
                }

                return false;
            }

            ApduCommand apdu(0x90, 0x34, 0x00, 0x01);
            result = smartCard->transmit(apdu.case4short(ecm, 0x00), response);
            if (result != SCARD_S_SUCCESS) {
                if (result == SCARD_W_RESET_CARD) {
                    if (retryCount > 4) {
                        return false;
                    }

                    cardReset = true;
                    ++retryCount;
                    goto retry;
                }

                return false;
            }

            if (!response.isSuccess()) {
                return false;
            }

            auto ecmData = response.getData();
            std::vector<uint8_t> ecmResponse(ecmData.begin() + 0x06, ecmData.end());
            std::vector<uint8_t> ecmInit(ecm.begin() + 0x04, ecm.begin() + 0x04 + 0x17);

            std::vector<uint8_t> plainData;
            plainData.insert(plainData.end(), kcl.begin(), kcl.end());
            plainData.insert(plainData.end(), ecmInit.begin(), ecmInit.end());

            sha256_t hash = SHA256::hash(plainData);

            for (size_t i = 0; i < hash.size(); i++) {
                hash[i] ^= ecmResponse[i];
            }

            std::copy(hash.begin(), hash.begin() + 0x10, output.odd.begin());
            std::copy(hash.begin() + 0x10, hash.begin() + 0x20, output.even.begin());
        }

        return true;
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }

    return false;
}

void AcasCard::setSmartCard(std::unique_ptr<ISmartCard> sc) {
    smartCard = std::move(sc);
}
