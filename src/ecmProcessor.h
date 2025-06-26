#pragma once
#include <span>
#include "acascard.h"
#include "extensionHeaderScrambling.h"
#include <future>
#include <queue>
#include "acascard.h"

class EcmProcessor {
public:
    EcmProcessor(MmtTlv::Acas::AcasCard& acasCard)
        : acasCard(acasCard)
    {
        workerThread = std::thread(&EcmProcessor::worker, this);
    }

    ~EcmProcessor() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        queueCv.notify_all();
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    bool init(bool reset = false);
    void onEcm(const std::vector<uint8_t>& ecm);
    std::optional<std::array<uint8_t, 16>> getDecryptionKey(MmtTlv::EncryptionFlag keyType);
    
private:
    using ECM = std::vector<uint8_t>;
    void worker();

private:
    MmtTlv::EncryptionFlag lastPayloadKeyType{ MmtTlv::EncryptionFlag::UNSCRAMBLED };
    std::queue<ECM> queue;
    std::condition_variable queueCv;
    std::vector<uint8_t> lastEcm;
    std::mutex queueMutex;
    std::mutex keyMutex;
    bool ecmReady{ false };
    struct MmtTlv::Acas::DecryptedEcm key;
    MmtTlv::Common::sha256_t kcl;
    MmtTlv::Acas::AcasCard& acasCard;
    bool stop{ false };
    std::thread workerThread;

#ifdef _WIN32
    void* mapView = nullptr;
    void* fileMapping = nullptr;
    void* ipcMutex = nullptr;
#endif

};