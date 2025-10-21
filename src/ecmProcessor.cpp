#include "ecmProcessor.h"
#include "config.h"

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

        MmtTlv::Acas::DecryptionKey key = {};
        MmtTlv::Acas::EcmResult ret;

        if (acasServerUrl.empty()) {
            try {
                ret = acasCard.ecm(current, key);

                if (ret == MmtTlv::Acas::EcmResult::CardResetError) {
                    acasCard.ecm(current, key);
                }
            }
            catch (const std::runtime_error& e) {
                std::cerr << "Error decrypting ECM: " << e.what() << std::endl;
                continue;
            }
        }
        else {
            /*
            MmtTlv::Acas::AcasClient client(config.acasServerUrl);
            MmtTlv::Acas::EcmRequest ecmRequest;
            ecmRequest.setSmartCardReaderName(config.smartCardReaderName);
            ecmRequest.setEcm(current);
            auto res = client.sendRequest(ecmRequest);
            if (res.getStatus() == MmtTlv::Acas::AcasServerResponseStatus::Success) {
                key = res.getData();
            } else {
                std::cerr << "Failed to get smart card readers from ACAS server: " << res.getMessage() << std::endl;
            }
            */
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
