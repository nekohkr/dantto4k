#include "smartCard.h"
#include <sstream>

bool LocalSmartCard::init() {
    LONG result = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);
    return result == SCARD_S_SUCCESS;
}

LocalSmartCard::~LocalSmartCard() {
    disconnect();

    if (hContext != 0) {
        SCardReleaseContext(hContext);
        hContext = 0;
    }
}

bool LocalSmartCard::isConnected() const {
    return (hCard != 0 && hCard != -1);
}

bool LocalSmartCard::isInited() const {
    return (hContext != 0 && hContext != -1);
}

void LocalSmartCard::connect() {
    LONG result;
    DWORD readersSize = SCARD_AUTOALLOCATE;
    std::string readerName;
    disconnect();

    if(smartCardReaderName == "") {
        char* readers = nullptr;
        result = SCardListReaders(hContext, nullptr, (LPSTR)&readers, &readersSize);
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
            SCardFreeMemory(hContext, readers);
        }
    }
    else {
        readerName = smartCardReaderName;
    }

    result = SCardConnect(hContext, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
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

    LONG result = SCardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
    while (result != SCARD_S_SUCCESS && retryCount < 5) {
        if (result == SCARD_W_RESET_CARD) {
            connect();
            return SCARD_W_RESET_CARD;
        }

        retryCount++;
        recvLength = 256;
        result = SCardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
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
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
        hCard = 0;
    }
}

void LocalSmartCard::beginTransaction() {
    if (!isConnected()) {
        return;
    }

    SCardBeginTransaction(hCard);
}

void LocalSmartCard::endTransaction() {
    SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
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
        oss << "Failed to connect to smart card (" << readerName << "): 0x" << std::hex << std::uppercase << result;
        throw std::runtime_error(oss.str());
    }
}

uint32_t RemoteSmartCard::transmit(const std::vector<uint8_t>& message, ApduResponse& response) {
    DWORD recvLength = 256;
    std::vector<uint8_t> recvBuffer(recvLength);
    int retryCount = 0;

    LONG result = client->scardTransmit(hCard, SCARD_PCI_T1, message.data(), static_cast<uint32_t>(message.size()), nullptr, recvBuffer.data(), &recvLength);
    while (result != SCARD_S_SUCCESS && retryCount < 5) {
        if (result == SCARD_W_RESET_CARD) {
            connect();
            return SCARD_W_RESET_CARD;
        }
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
