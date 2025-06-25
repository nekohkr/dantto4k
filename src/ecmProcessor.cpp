#include "ecmProcessor.h"

bool EcmProcessor::init(bool reset)
{
    // Sharing KCL with other processes is not yet implemented on Linux.
#ifdef _WIN32
    fileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 4096, "dantto4k_acas_kcl");
    if (fileMapping == nullptr) {
        std::cerr << "Failed to create file mapping: " << GetLastError() << std::endl;
        return false;
    }

    bool alreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);
    mapView = MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (mapView == nullptr) {
        std::cerr << "Failed to map view of file: " << GetLastError() << std::endl;
        CloseHandle(fileMapping);
        return false;
    }

    ipcMutex = CreateMutex(nullptr, FALSE, "dantto4k_acas_kcl_mutex");
    if (ipcMutex == nullptr) {
        std::cerr << "Failed to create mutex: " << GetLastError() << std::endl;
        UnmapViewOfFile(mapView);
        CloseHandle(fileMapping);
        return false;
    }

    WaitForSingleObject(ipcMutex, INFINITE);
    uint8_t* base = static_cast<uint8_t*>(mapView);
    if (alreadyExists && !reset) {
        std::copy(base, base + 32, kcl.begin());
    }
    else {
        acasCard.getA0AuthKcl(kcl);
        std::copy(kcl.begin(), kcl.end(), base);
    }
    ReleaseMutex(ipcMutex);
#endif

    return false;
}

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


            current = std::move(queue.front());
        }

        MmtTlv::Acas::DecryptedEcm key;
#ifndef _WIN32
        acasCard.getA0AuthKcl(kcl);
#endif
        auto ret = acasCard.decryptEcm(kcl, current, key);
        if (ret == MmtTlv::Acas::DecryptEcmResult::CardResetError) {
            init(true);
#ifndef _WIN32
            acasCard.getA0AuthKcl(kcl);
#endif
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

                if (stop) {
                    break;
                }
            }
        }
    }
}
