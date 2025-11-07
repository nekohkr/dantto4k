#include "acasHandler.h"
#include "config.h"
#include "mmtp.h"
#include "aesCtrCipher.h"

AcasHandler::AcasHandler() {
    acasCard = std::make_unique<AcasCard>();
    workerThread = std::thread(&AcasHandler::worker, this);
}

AcasHandler::~AcasHandler() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        running = false;
    }

    queueCv.notify_all();
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

bool AcasHandler::onEcm(const std::vector<uint8_t>& ecm) {
    if (lastEcm == ecm) {
        return true;
    }
    lastEcm = ecm;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(ecm);
    }
    queueCv.notify_one();
    ecmReady = true;
    return true;
}

bool AcasHandler::decrypt(MmtTlv::Mmtp& mmtp) {
    auto key = getDecryptionKey(mmtp.extensionHeaderScrambling->encryptionFlag);
    if (!key) {
        return false;
    }

    std::array<uint8_t, 16> iv{};
    uint16_t packetIdBe = MmtTlv::Common::swapEndian16(mmtp.packetId);
    uint32_t packetSequenceNumberBe = MmtTlv::Common::swapEndian32(mmtp.packetSequenceNumber);
    memcpy(iv.data(), &packetIdBe, 2);
    memcpy(iv.data() + 2, &packetSequenceNumberBe, 4);

    try {
        AESCtrCipher aes(*key);
        aes.setIv(iv);
        aes.decrypt(mmtp.payload.data() + 8, static_cast<int>(mmtp.payload.size() - 8), mmtp.payload.data() + 8);
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
    return true;
}

void AcasHandler::setSmartCard(std::unique_ptr<ISmartCard> sc) {
    acasCard->setSmartCard(std::move(sc));
}

std::optional<std::array<uint8_t, 16>> AcasHandler::getDecryptionKey(MmtTlv::EncryptionFlag keyType) {
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

void AcasHandler::worker() {
    while (true) {
        ECM current;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCv.wait(lock, [&]() {
                return !queue.empty() || !running;
                });

            if (!running) {
                break;
            }

            current = std::move(queue.front());
        }

        AcasCard::DecryptionKey key = {};
        acasCard->ecm(current, key);

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
