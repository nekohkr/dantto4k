#include "smartcard.h"
#include <sstream>

namespace MmtTlv::Acas {

bool SmartCard::init() {
    LONG result = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);
    return result == SCARD_S_SUCCESS;
}

bool SmartCard::isConnected() const {
    if (hCard) {
        return true;
    }

    return false;
}

void SmartCard::connect() {
    LONG result;
    DWORD readersSize = SCARD_AUTOALLOCATE;
    std::string readerName;
    disconnect();

    if(smartCardReaderName == "") {
        char* readers = nullptr;
        result = SCardListReaders(hContext, nullptr, (LPSTR)&readers, &readersSize);
        if (result != SCARD_S_SUCCESS) {
            std::ostringstream oss;
            oss << "Failed to list smart card readers: " << std::showbase << std::hex << result;
            throw std::runtime_error(oss.str());
        }

        readerName = readers;
        SCardFreeMemory(hContext, readers);
    }
    else {
        readerName = smartCardReaderName;
    }


    result = SCardConnect(hContext, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (result != SCARD_S_SUCCESS) {
        std::ostringstream oss;
        oss << "Failed to connect to smart card reader: " << std::showbase << std::hex << result;
        throw std::runtime_error(oss.str());
    }
}

uint32_t SmartCard::transmit(const std::vector<uint8_t>& message, ApduResponse& response) {
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

void SmartCard::disconnect() {
    if (hCard != 0) {
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
        hCard = 0;
    }
}

void SmartCard::release() {
    disconnect();

    if (hContext != 0) {
		SCardReleaseContext(hContext);
        hContext = 0;
    }
}

void SmartCard::beginTransaction() {
    if (!isConnected()) {
        throw std::runtime_error("smart card not connected");
    }

    LONG result = SCardBeginTransaction(hCard);
    if (result != SCARD_S_SUCCESS) {
        throw std::runtime_error("SCardBeginTransaction failed");
    }
}

void SmartCard::endTransaction() {
    SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
}

}