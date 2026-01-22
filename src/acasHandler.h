#pragma once
#include <span>
#include <future>
#include <queue>
#include "extensionHeaderScrambling.h"
#include "acasCard.h"
#include "smartCard.h"
#include "casHandler.h"
#include "aesCtrCipher.h"

class AcasHandler : public MmtTlv::CasHandler {
public:
    AcasHandler();
    ~AcasHandler();
    bool onEcm(const std::vector<uint8_t>& ecm) override;
    bool decrypt(MmtTlv::Mmtp& mmtp) override;
    void clear() override;
    void setSmartCard(std::unique_ptr<ISmartCard> sc);

private:
    void worker();
    std::optional<std::array<uint8_t, 16>> getDecryptionKey(MmtTlv::EncryptionFlag keyType);

    using Task = std::pair<uint64_t, std::vector<uint8_t>>;
    MmtTlv::EncryptionFlag lastPayloadKeyType{ MmtTlv::EncryptionFlag::UNSCRAMBLED };
    std::queue<Task> queue;
    std::condition_variable queueCv;
    std::vector<uint8_t> lastEcm;
    std::mutex queueMutex;
    std::mutex keyMutex;
    bool ecmReady{false};
    std::unique_ptr<AcasCard> acasCard;
    AcasCard::DecryptionKey key;
    bool running{true};
    std::thread workerThread;
    AESCtrCipher aes;
    std::array<uint8_t, 16> lastKey{};
    bool hasAESNI = false;
    std::atomic<uint64_t> generation{0};
    bool processing{false};

};
