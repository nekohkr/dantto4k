#include "smartCard.h"
#include <sstream>
#include "config.h"

bool LocalSmartCard::init() {
#ifdef WIN32
    LONG result = pSCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);
#else
    LONG result = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);
#endif
    
    return result == SCARD_S_SUCCESS;
}

LocalSmartCard::LocalSmartCard() {
#ifdef WIN32
    HMODULE hWinSCard = LoadLibraryA(config.customWinscardDLL.empty() ? "winscard.dll" : config.customWinscardDLL.c_str());
    if (!hWinSCard) {
        if (config.customWinscardDLL.empty()) {
            throw std::runtime_error("Failed to load winscard.dll");
        }
        else {
            throw std::runtime_error("Failed to load winscard.dll from specified path: " + config.customWinscardDLL);
        }
    }

    pSCardEstablishContext = reinterpret_cast<FnSCardEstablishContext>(GetProcAddress(hWinSCard, "SCardEstablishContext"));
    if (!pSCardEstablishContext) {
        throw std::runtime_error("Failed to get address of SCardEstablishContext");
    }
    pSCardReleaseContext = reinterpret_cast<FnSCardReleaseContext>(GetProcAddress(hWinSCard, "SCardReleaseContext"));
    if (!pSCardReleaseContext) {
        throw std::runtime_error("Failed to get address of SCardReleaseContext");
    }
    pSCardConnect = reinterpret_cast<FnSCardConnect>(GetProcAddress(hWinSCard, "SCardConnectA"));
    if (!pSCardConnect) {
        throw std::runtime_error("Failed to get address of SCardConnectA");
    }
    pSCardDisconnect = reinterpret_cast<FnSCardDisconnect>(GetProcAddress(hWinSCard, "SCardDisconnect"));
    if (!pSCardDisconnect) {
        throw std::runtime_error("Failed to get address of SCardDisconnect");
    }

    pSCardBeginTransaction = reinterpret_cast<FnSCardBeginTransaction>(GetProcAddress(hWinSCard, "SCardBeginTransaction"));
    pSCardEndTransaction = reinterpret_cast<FnSCardEndTransaction>(GetProcAddress(hWinSCard, "SCardEndTransaction"));

    pSCardTransmit = reinterpret_cast<FnSCardTransmit>(GetProcAddress(hWinSCard, "SCardTransmit"));
    if (!pSCardTransmit) {
        throw std::runtime_error("Failed to get address of SCardTransmit");
    }
    pSCardListReaders = reinterpret_cast<FnSCardListReaders>(GetProcAddress(hWinSCard, "SCardListReadersA"));
    if (!pSCardListReaders) {
        throw std::runtime_error("Failed to get address of SCardListReadersA");
    }
    pSCardFreeMemory = reinterpret_cast<FnSCardFreeMemory>(GetProcAddress(hWinSCard, "SCardFreeMemory"));
    if (!pSCardFreeMemory) {
        throw std::runtime_error("Failed to get address of SCardFreeMemory");
    }
#endif
}

LocalSmartCard::~LocalSmartCard() {
    disconnect();

    if (hContext != 0) {
#ifdef WIN32
        pSCardReleaseContext(hContext);
#else
        SCardReleaseContext(hContext);
#endif
        hContext = 0;
    }
}

bool LocalSmartCard::isConnected() const {
    return (hCard != 0 && hCard != -1);
}

bool LocalSmartCard::isInited() const {
    return (hContext != 0 && hContext != -1);
}

std::vector<std::string> LocalSmartCard::getReaders() const {
    DWORD readersSize = 0;
#ifdef WIN32
    uint32_t result = pSCardListReaders(hContext, nullptr, nullptr, &readersSize);
#else
    uint32_t result = SCardListReaders(hContext, nullptr, nullptr, &readersSize);
#endif
    if (result != SCARD_S_SUCCESS) {
        if (result == SCARD_E_NO_READERS_AVAILABLE) {
            throw std::runtime_error("No smart card readers are available");
        }

        throw std::runtime_error(
            "Failed to list smart card readers: " +
            [&result]() {
                std::ostringstream oss;
                oss << std::showbase << std::hex << result;
                return oss.str();
            }()
        );
    }

    std::vector<char> readersBuffer(readersSize);

#ifdef WIN32
    result = pSCardListReaders(hContext, nullptr, readersBuffer.data(), &readersSize);
#else
    result = SCardListReaders(hContext, nullptr, readersBuffer.data(), &readersSize);
#endif
    if (result != SCARD_S_SUCCESS) {
        if (result == SCARD_E_NO_READERS_AVAILABLE) {
            throw std::runtime_error("No smart card readers are available");
        }

        throw std::runtime_error(
            "Failed to list smart card readers: " +
            [&result]() {
                std::ostringstream oss;
                oss << std::showbase << std::hex << result;
                return oss.str();
            }()
        );
    }

    std::vector<std::string> readers;
    const char* reader = readersBuffer.data();
    while (*reader != '\0') {
        readers.emplace_back(reader);
        reader += strlen(reader) + 1;
    }

    return readers;
}

void LocalSmartCard::connect() {
    LONG result;
    DWORD readersSize = SCARD_AUTOALLOCATE;
    std::string readerName;
    disconnect();

    if(smartCardReaderName == "") {
        char* readers = nullptr;
#ifdef WIN32
        result = pSCardListReaders(hContext, nullptr, (LPSTR)&readers, &readersSize);
#else
        result = SCardListReaders(hContext, nullptr, (LPSTR)&readers, &readersSize);
#endif
        if (result != SCARD_S_SUCCESS) {
            if (result == SCARD_E_NO_READERS_AVAILABLE) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                throw std::runtime_error("No smart card readers are available");
            }

            std::ostringstream oss;
            oss << "Failed to list smart card readers: " << std::showbase << std::hex << result;
            throw std::runtime_error(oss.str());
        }

        if (readers != nullptr) {
            readerName = readers;
#ifdef WIN32
            pSCardFreeMemory(hContext, readers);
#else
            SCardFreeMemory(hContext, readers);
#endif
        }
    }
    else {
        readerName = smartCardReaderName;
    }

#ifdef WIN32
    result = pSCardConnect(hContext, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
#else
    result = SCardConnect(hContext, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
#endif
    if (result != SCARD_S_SUCCESS) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (result == SCARD_E_NO_SMARTCARD) {
            throw std::runtime_error("No smart card inserted in reader: " + readerName);
        }

        std::ostringstream oss;
        oss << "Failed to connect to smart card (" << readerName << "): 0x" << std::hex << std::uppercase << result;
        throw std::runtime_error(oss.str());
    }
}

uint32_t LocalSmartCard::transmit(const std::vector<uint8_t>& message, ApduResponse& response) {
    DWORD recvLength = 256;
    std::vector<uint8_t> recvBuffer(recvLength);
    int retryCount = 0;

#ifdef WIN32
    LONG result = pSCardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
#else
    LONG result = SCardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
#endif
    while (result != SCARD_S_SUCCESS && retryCount < 5) {
        if (result == SCARD_W_RESET_CARD || result == SCARD_E_NOT_TRANSACTED) {
            hCard = 0;
            return SCARD_W_RESET_CARD;
        }

        retryCount++;
        recvLength = 256;

#ifdef WIN32
        result = pSCardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
#else
        result = SCardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
#endif
    }

    if (result != SCARD_S_SUCCESS) {
        return result;
    }

    uint8_t sw1 = recvBuffer[recvLength - 2];
    uint8_t sw2 = recvBuffer[recvLength - 1];

    std::vector<uint8_t> data(recvBuffer.begin(), recvBuffer.begin() + recvLength - 2);
    response = ApduResponse(sw1, sw2, data);
    return result;
}

void LocalSmartCard::setSmartCardReaderName(const std::string& name) {
    smartCardReaderName = name;
}

std::string LocalSmartCard::getSmartCardReaderName() const {
    return smartCardReaderName;
}

void LocalSmartCard::disconnect() {
    if (hCard != 0) {
#ifdef WIN32
        pSCardDisconnect(hCard, SCARD_LEAVE_CARD);
#else
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
#endif
        hCard = 0;
    }
}

void LocalSmartCard::beginTransaction() {
    if (!isConnected()) {
        return;
    }

#ifdef WIN32
    if (pSCardBeginTransaction != nullptr) {
        pSCardBeginTransaction(hCard);
    }
#else
    SCardBeginTransaction(hCard);
#endif
}

void LocalSmartCard::endTransaction() {
#ifdef WIN32
    if (pSCardEndTransaction != nullptr) {
        pSCardEndTransaction(hCard, SCARD_LEAVE_CARD);
    }
#else
    SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
#endif
}

RemoteSmartCard::RemoteSmartCard(std::string casProxyHost, uint16_t port) {
    client = std::make_unique<CasProxyClient>(casProxyHost, port);
    client->connect();
}

bool RemoteSmartCard::init() {
    LONG result = client->scardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);
    return result == SCARD_S_SUCCESS;
}

RemoteSmartCard::~RemoteSmartCard() {
    client->close();
}

bool RemoteSmartCard::isConnected() const {
    return (hCard != 0 && hCard != -1);
}

bool RemoteSmartCard::isInited() const {
    return (hContext != 0 && hContext != -1);
}

std::vector<std::string> RemoteSmartCard::getReaders() const {
    DWORD readersSize = 0;
    uint32_t result = client->scardListReaders(hContext, nullptr, nullptr, &readersSize);
    if (result != SCARD_S_SUCCESS) {
        if (result == SCARD_E_NO_READERS_AVAILABLE) {
            throw std::runtime_error("No smart card readers are available");
        }

        throw std::runtime_error(
            "Failed to list smart card readers: " +
            [&result]() {
                std::ostringstream oss;
                oss << std::showbase << std::hex << result;
                return oss.str();
            }()
        );
    }

    std::vector<char> readersBuffer(readersSize);

    result = client->scardListReaders(hContext, nullptr, readersBuffer.data(), &readersSize);
    if (result != SCARD_S_SUCCESS) {
        if (result == SCARD_E_NO_READERS_AVAILABLE) {
            throw std::runtime_error("No smart card readers are available");
        }

        throw std::runtime_error(
            "Failed to list smart card readers: " +
            [&result]() {
                std::ostringstream oss;
                oss << std::showbase << std::hex << result;
                return oss.str();
            }()
        );
    }

    std::vector<std::string> readers;
    const char* reader = readersBuffer.data();
    while (*reader != '\0') {
        readers.emplace_back(reader);
        reader += strlen(reader) + 1;
    }

    return readers;
}

void RemoteSmartCard::connect() {
    LONG result;
    DWORD readersSize = SCARD_AUTOALLOCATE;
    std::string readerName;
    
    disconnect();
    
    if(smartCardReaderName == "") {
        char* readers = nullptr;
        result = client->scardListReaders(hContext, nullptr, (LPSTR)&readers, &readersSize);
        if (result != SCARD_S_SUCCESS) {
            if (result == SCARD_E_NO_READERS_AVAILABLE) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                throw std::runtime_error("No smart card readers are available");
            }

            // CasProxyServer error occurred
            if (result == SCARD_E_INVALID_HANDLE || result == SCARD_F_INTERNAL_ERROR) {
                hContext = 0;
                hCard = 0;
            }

            std::ostringstream oss;
            oss << "Failed to list smart card readers: " << std::showbase << std::hex << result;
            throw std::runtime_error(oss.str());
        }

        if (readers != nullptr) {
            readerName = readers;
            client->scardFreeMemory(hContext, readers);
        }

        if (readerName.empty()) {
            throw std::runtime_error("No smart card readers are available");
        }
    }
    else {
        readerName = smartCardReaderName;
    }

    result = client->scardConnect(hContext, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (result != SCARD_S_SUCCESS) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (result == SCARD_E_NO_SMARTCARD) {
            throw std::runtime_error("No smart card inserted in reader: " + readerName);
        }

        std::ostringstream oss;
        oss << "Failed to connect to smart card (" << readerName << "): " << std::showbase << std::hex << std::uppercase << result;
        throw std::runtime_error(oss.str());
    }
}

uint32_t RemoteSmartCard::transmit(const std::vector<uint8_t>& message, ApduResponse& response) {
    DWORD recvLength = 256;
    std::vector<uint8_t> recvBuffer(recvLength);
    int retryCount = 0;

    LONG result = client->scardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
    while (result != SCARD_S_SUCCESS && retryCount < 5) {
        if (result == SCARD_W_RESET_CARD || result == SCARD_E_NOT_TRANSACTED) {
            hCard = 0;
            return SCARD_W_RESET_CARD;
        }

        // CasProxyServer error occurred
        if (result == SCARD_E_INVALID_HANDLE || result == SCARD_F_INTERNAL_ERROR) {
            hContext = 0;
            hCard = 0;
            return result;
        }

        retryCount++;
        recvLength = 256;
        result = client->scardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
    }

    if (result != SCARD_S_SUCCESS) {
        return result;
    }
    
    recvBuffer.resize(recvLength);

    uint8_t sw1 = recvBuffer[recvLength - 2];
    uint8_t sw2 = recvBuffer[recvLength - 1];

    std::vector<uint8_t> data(recvBuffer.begin(), recvBuffer.begin() + recvLength - 2);
    response = ApduResponse(sw1, sw2, data);
    return result;
}

void RemoteSmartCard::setSmartCardReaderName(const std::string& name) {
    smartCardReaderName = name;
}

std::string RemoteSmartCard::getSmartCardReaderName() const {
    return smartCardReaderName;
}

void RemoteSmartCard::disconnect() {
    if (hCard != 0) {
        client->scardDisconnect(hCard, SCARD_LEAVE_CARD);
        hCard = 0;
    }
}

void RemoteSmartCard::beginTransaction() {
    if (!isConnected()) {
        return;
    }

    client->scardBeginTransaction(hCard);
}

void RemoteSmartCard::endTransaction() {
    if (!isConnected()) {
        return;
    }

    client->scardEndTransaction(hCard, SCARD_LEAVE_CARD);
}
