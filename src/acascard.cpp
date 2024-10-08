#include "acascard.h"
#include <random>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#pragma comment(lib, "Winscard.lib")

AcasCard::AcasCard(SmartCard* smartcard)
{
	this->smartcard = smartcard;
}

void AcasCard::init()
{
    ApduCommand apdu(0x90, 0x30, 0x00, 0x01);
    smartcard->transmit(apdu.case2short(0x00));
}

std::vector<uint8_t> AcasCard::getA0AuthKcl()
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
    auto response = smartcard->transmit(apdu.case4short(data, 0x00));

    if (!response.isSuccess()) {
        throw std::runtime_error("A0 auth failed");
    }

    auto a0data = response.getData();
    std::vector<uint8_t> a0response(a0data.begin() + 0x06, a0data.begin() + 0x06 + 0x08);
    std::vector<uint8_t> a0hash(a0data.begin() + 0x0e, a0data.end());

    std::vector<uint8_t> painKcl;
    painKcl.insert(painKcl.end(), std::begin(masterKey), std::end(masterKey));
    painKcl.insert(painKcl.end(), a0init.begin(), a0init.end());
    painKcl.insert(painKcl.end(), a0response.begin(), a0response.end());

    std::vector<uint8_t> kcl(32);
    {
        EVP_MD_CTX* mdctx;
        unsigned int digest_len = (unsigned int)kcl.size();
        mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(mdctx, painKcl.data(), painKcl.size());
        EVP_DigestFinal_ex(mdctx, kcl.data(), &digest_len);
        EVP_MD_CTX_free(mdctx);
    }

    std::vector<uint8_t> plainData;
    plainData.insert(plainData.end(), kcl.begin(), kcl.end());
    plainData.insert(plainData.end(), a0init.begin(), a0init.end());

    std::vector<uint8_t> hash(32);
    {
        EVP_MD_CTX* mdctx;
        unsigned int digest_len = (unsigned int)kcl.size();
        mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(mdctx, plainData.data(), plainData.size());
        EVP_DigestFinal_ex(mdctx, hash.data(), &digest_len);
        EVP_MD_CTX_free(mdctx);
    }

    if (hash != a0hash) {
        throw std::runtime_error("A0 hash did not match");
    }

    return kcl;
}

DecryptedEcm AcasCard::decryptEcm(std::vector<uint8_t>& ecm)
{
    if (decryptedEcmMap.find(ecm) != decryptedEcmMap.end()) {
        return decryptedEcmMap[ecm];
    }

    auto kcl = getA0AuthKcl();

    ApduCommand apdu(0x90, 0x34, 0x00, 0x01);
    auto response = smartcard->transmit(apdu.case4short(ecm, 0x00));

    if (!response.isSuccess()) {
        throw std::runtime_error("ECM request failed");
    }

    auto ecmData = response.getData();
    std::vector<uint8_t> ecmResponse(ecmData.begin() + 0x06, ecmData.end());
    std::vector<uint8_t> ecmInit(ecm.begin() + 0x04, ecm.begin() + 0x04 + 0x17);

    std::vector<uint8_t> plainData;
    plainData.insert(plainData.end(), kcl.begin(), kcl.end());
    plainData.insert(plainData.end(), ecmInit.begin(), ecmInit.end());

    std::vector<uint8_t> hash(32);
    {
        EVP_MD_CTX* mdctx;
        unsigned int digest_len = (unsigned int)kcl.size();
        mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(mdctx, plainData.data(), plainData.size());
        EVP_DigestFinal_ex(mdctx, hash.data(), &digest_len);
        EVP_MD_CTX_free(mdctx);
    }

    for (int i = 0; i < hash.size(); i++) {
        hash.data()[i] ^= ecmResponse[i];
    }

    DecryptedEcm decryptedEcm;
    memcpy(decryptedEcm.odd, hash.data(), 0x10);
    memcpy(decryptedEcm.even, hash.data() + 0x10, 0x10);

    decryptedEcmMap[ecm] = decryptedEcm;
    lastDecryptedEcm = decryptedEcm;
    ready = true;

    return decryptedEcm;
}
