#pragma once
#include <span>
#include "acascard.h"
#include "extensionHeaderScrambling.h"
#include <future>
#include <queue>
#include "acascard.h"
#include "namedLock.h"

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

    void onEcm(const std::vector<uint8_t>& ecm);
    void setAcasServerUrl(const std::string& url) {
        acasServerUrl = url;
    }
    std::optional<std::array<uint8_t, 16>> getDecryptionKey(MmtTlv::EncryptionFlag keyType);
    
private:
    void worker();

private:
    using ECM = std::vector<uint8_t>;
    MmtTlv::EncryptionFlag lastPayloadKeyType{ MmtTlv::EncryptionFlag::UNSCRAMBLED };
    std::queue<ECM> queue;
    std::condition_variable queueCv;
    std::vector<uint8_t> lastEcm;
    std::mutex queueMutex;
    std::mutex keyMutex;
    bool ecmReady{ false };
    struct MmtTlv::Acas::DecryptedEcm key;
    MmtTlv::Acas::AcasCard& acasCard;
    bool stop{ false };
    std::thread workerThread;
    NamedLock ipcLock{ "dantto4k_acas" };
    std::string acasServerUrl;
};