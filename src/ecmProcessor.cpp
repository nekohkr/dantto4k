#include "ecmProcessor.h"


void EcmProcessor::onEcm(const std::vector<uint8_t>& ecm)
{
    if (lastEcm == ecm) {
        return;
    }
    lastEcm = ecm;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(ecm);
    }
    queueCv.notify_one();
    ecmReady = true;
}

std::optional<std::array<uint8_t, 16>> EcmProcessor::getDecryptionKey(MmtTlv::EncryptionFlag keyType)
{
    if (!ecmReady) {
        return std::nullopt; 
    }

    if (lastPayloadKeyType != keyType) {
        std::unique_lock<std::mutex> lock(queueMutex);
        bool ready = queueCv.wait_for(lock, std::chrono::seconds(10), [&]() {
            return queue.empty();
            });
        if (!ready) {
            // timeout
            return std::nullopt;
        }
    }

    lastPayloadKeyType = keyType;

    {
        std::lock_guard<std::mutex> lock(keyMutex);
        if (keyType == MmtTlv::EncryptionFlag::EVEN) {
            return key.even;
        }
        else {
            return key.odd;
        }
    }
}

void EcmProcessor::worker()
{
    while (true) {
        ECM current;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCv.wait(lock, [&]() {
                return !queue.empty() || stop;
            });

            if (stop) {
                break;
            }

            current = std::move(queue.front());
        }

        MmtTlv::Common::sha256_t kcl;
        MmtTlv::Acas::DecryptedEcm key;
        MmtTlv::Acas::DecryptEcmResult ret;
        {
            std::lock_guard<NamedLock> lock(ipcLock);
            acasCard.getA0AuthKcl(kcl);
            ret = acasCard.decryptEcm(kcl, current, key);
        }
        
        if (ret == MmtTlv::Acas::DecryptEcmResult::CardResetError) {
            std::lock_guard<NamedLock> lock(ipcLock);
            acasCard.getA0AuthKcl(kcl);
            acasCard.decryptEcm(kcl, current, key);
        }

        {
            std::lock_guard<std::mutex> lock(keyMutex);
            this->key = key;
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.pop();

            if (queue.empty()) {
                queueCv.notify_all();
            }
        }
    }
}
