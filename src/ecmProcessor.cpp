#include "ecmProcessor.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

namespace {

void splitHex(const std::string& hex, std::array<uint8_t, 16>& even, std::array<uint8_t, 16>& odd) {
    for (size_t i = 0; i < 32; i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        even[i/2] = byte;
    }
    for (size_t i = 32; i < 64; i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        odd[(i - 32) / 2] = byte;
    }
}

std::optional<std::tuple<std::string, std::string, std::string>> splitUrl(const std::string& fullUrl) {
    std::regex urlRegex(R"((https?)://([^/]+)(/.*))");
    std::smatch match;
    if (std::regex_match(fullUrl, match, urlRegex)) {
        std::string scheme = match[1];
        std::string host = match[2];
        std::string path = match[3];
        return std::make_tuple(scheme, host, path);
    }
    else {
        return std::nullopt;
    }
}

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

            if (stop) {
                break;
            }

            current = std::move(queue.front());
        }

        MmtTlv::Acas::DecryptedEcm key = {};
        MmtTlv::Acas::DecryptEcmResult ret;

        if (acasServerUrl.empty()) {
            try {
                ret = acasCard.decryptEcm(current, key);

                if (ret == MmtTlv::Acas::DecryptEcmResult::CardResetError) {
                    acasCard.decryptEcm(current, key);
                }
            }
            catch (const std::runtime_error& e) {
                std::cerr << "Error decrypting ECM: " << e.what() << std::endl;
                continue;
            }
        }
        else {
            while (1) {
                auto result = splitUrl(acasServerUrl);
                if (!result) {
                    break;
                }

                auto [scheme, host, path] = *result;

                httplib::Client cli(scheme + "://" + host);

                auto to_hex = [](const std::vector<uint8_t>& data) {
                    std::ostringstream oss;
                    for (auto b : data) {
                        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)b;
                    }
                    return oss.str();
                    };

                std::string keyHex = to_hex(current);
                std::string body = "ecm=" + keyHex;
                auto res = cli.Post(path, body, "application/x-www-form-urlencoded");

                if (res.error() != httplib::Error::Success) {
                    std::cerr << "Failed to send ECM to ACAS server: " << res.error() << std::endl;
                    break;
                }

                if (res->status != 200) {
                    std::cerr << "ACAS server returned error: " << res->status << std::endl;
                    break;
                }

                splitHex(res->body, key.even, key.odd);
                break;
            }
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
