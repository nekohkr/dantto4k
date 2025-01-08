#include "acascard.h"
#include <random>
#include <openssl/evp.h>
#pragma comment(lib, "Winscard.lib")

namespace MmtTlv::Acas {

AcasCard::AcasCard(std::shared_ptr<SmartCard> smartCard)
    : smartCard(smartCard)
{
}

void AcasCard::init()
{
    ApduCommand apdu(0x90, 0x30, 0x00, 0x01);
    smartCard->transmit(apdu.case2short(0x00));
}

Common::sha256_t AcasCard::getA0AuthKcl() const
{
    std::default_random_engine engine(std::random_device{}());
    std::uniform_int_distribution<int> distrib(0, 255);

    std::vector<uint8_t> data = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x8A, 0xF7 };
    std::vector<uint8_t> a0init(8);
    for (size_t i = 0; i < 8; ++i) {
        a0init.data()[i] = static_cast<uint8_t>(distrib(engine));
    }

    data.insert(data.end(), a0init.begin(), a0init.end());

    ApduCommand apdu(0x90, 0xA0, 0x00, 0x01);
    auto response = smartCard->transmit(apdu.case4short(data, 0x00));

    if (!response.isSuccess()) {
        throw std::runtime_error("A0 auth failed");
    }

    auto a0data = response.getData();
    std::vector<uint8_t> a0response(a0data.begin() + 0x06, a0data.begin() + 0x06 + 0x08);
    std::vector<uint8_t> a0hash(a0data.begin() + 0x0e, a0data.end());

    std::vector<uint8_t> plainKcl;
    plainKcl.insert(plainKcl.end(), std::begin(masterKey), std::end(masterKey));
    plainKcl.insert(plainKcl.end(), a0init.begin(), a0init.end());
    plainKcl.insert(plainKcl.end(), a0response.begin(), a0response.end());

    Common::sha256_t kcl = Common::sha256(plainKcl);

    std::vector<uint8_t> plainData;
    plainData.insert(plainData.end(), kcl.begin(), kcl.end());
    plainData.insert(plainData.end(), a0init.begin(), a0init.end());

    Common::sha256_t hash = Common::sha256(plainData);
    
    if (!std::equal(hash.begin(), hash.end(), a0hash.begin())) {
        throw std::runtime_error("A0 hash did not match");
    }

    return kcl;
}

DecryptedEcm AcasCard::decryptEcm(std::vector<uint8_t>& ecm)
{
    if (decryptedEcmMap.find(ecm) != decryptedEcmMap.end()) {
        return decryptedEcmMap[ecm];
    }

    Common::sha256_t kcl = getA0AuthKcl();

    ApduCommand apdu(0x90, 0x34, 0x00, 0x01);
    auto response = smartCard->transmit(apdu.case4short(ecm, 0x00));

    if (!response.isSuccess()) {
        throw std::runtime_error("ECM request failed");
    }

    auto ecmData = response.getData();
    std::vector<uint8_t> ecmResponse(ecmData.begin() + 0x06, ecmData.end());
    std::vector<uint8_t> ecmInit(ecm.begin() + 0x04, ecm.begin() + 0x04 + 0x17);

    std::vector<uint8_t> plainData;
    plainData.insert(plainData.end(), kcl.begin(), kcl.end());
    plainData.insert(plainData.end(), ecmInit.begin(), ecmInit.end());

    Common::sha256_t hash = Common::sha256(plainData);

    for (int i = 0; i < hash.size(); i++) {
        hash[i] ^= ecmResponse[i];
    }

    DecryptedEcm decryptedEcm{};
    std::copy(hash.begin(), hash.begin() + 0x10, decryptedEcm.odd.begin());
    std::copy(hash.begin() + 0x10, hash.begin() + 0x20, decryptedEcm.even.begin());

    decryptedEcmMap[ecm] = decryptedEcm;
    lastDecryptedEcm = decryptedEcm;
    ready = true;

    return decryptedEcm;
}

void AcasCard::clear()
{
    ready = false;
    decryptedEcmMap.clear();
}

}