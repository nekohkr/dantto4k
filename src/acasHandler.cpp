#include "acasHandler.h"
#include "config.h"
#include "mmtp.h"
#include "aes.h"

AcasHandler::AcasHandler() {
    acasCard = std::make_unique<AcasCard>();
    workerThread = std::thread(&AcasHandler::worker, this);
    hasAESNI = AESCtrCipher::hasAESNI();
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
        queue.push({ generation.load(std::memory_order_relaxed), ecm });
        processing = true;
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

    if (hasAESNI) { [[likely]]
        if (lastKey != *key) { [[unlikely]]
            aes.setKey(*key);
            lastKey = *key;
        }
        aes.setIv(iv);
        aes.decrypt(mmtp.payload.data() + 8, static_cast<int>(mmtp.payload.size() - 8), mmtp.payload.data() + 8);
    }
    else {
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, (*key).data(), iv.data());
        AES_CTR_xcrypt_buffer(&ctx, mmtp.payload.data() + 8, static_cast<int>(mmtp.payload.size() - 8));
    }

    return true;
}

void AcasHandler::clear() {
    ecmReady = false;

    std::queue<Task> empty;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.swap(empty);
        processing = false;
    }
    lastEcm.clear();
    generation.fetch_add(1, std::memory_order_relaxed);
    queueCv.notify_all();
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
            return !processing;
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
        Task current;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCv.wait(lock, [&]() {
                return !queue.empty() || !running;
            });

            if (!running) {
                break;
            }

            current = std::move(queue.front());
            queue.pop();
        }

        AcasCard::DecryptionKey key = {};
        acasCard->ecm(current.second, key);

        if (generation.load(std::memory_order_relaxed) == current.first) {
            std::lock_guard<std::mutex> lock(keyMutex);
            this->key = key;
        }
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (queue.empty()) {
                processing = false;
                queueCv.notify_all();
            }
        }
    }
}
