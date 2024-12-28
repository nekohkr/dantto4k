#include "SmartCard.h"

namespace MmtTlv::Acas {

bool SmartCard::init() {
    LONG result = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);
    return result == SCARD_S_SUCCESS;
}

bool SmartCard::isConnected()
{
    if (hCard) {
        return true;
    }

    return false;
}

void SmartCard::connect() {
    LONG result;
    DWORD readersSize = SCARD_AUTOALLOCATE;
    LPTSTR readers = nullptr;

    disconnect();

    result = SCardListReaders(hContext, nullptr, (LPTSTR)&readers, &readersSize);
    if (result != SCARD_S_SUCCESS) {
        throw std::runtime_error("Failed to list smart card readers. (result: " + std::to_string(result) + ")");
    }

    std::wstring readerName(readers);
    SCardFreeMemory(hContext, readers);

    result = SCardConnect(hContext, readerName.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (result != SCARD_S_SUCCESS) {
        throw std::runtime_error("Failed to connect to smart card. (result: " + std::to_string(result) + ")");
    }
}

ApduResponse SmartCard::transmit(const std::vector<uint8_t>& sendData) {
    DWORD recvLength = 256;
    std::vector<uint8_t> recvBuffer(recvLength);
    int retryCount = 0;

    LONG result = SCardTransmit(hCard, SCARD_PCI_T1, sendData.data(), sendData.size(), nullptr, recvBuffer.data(), &recvLength);
    while (result != SCARD_S_SUCCESS && retryCount < 10) {
        retryCount++;
        try {
            connect();
        }
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            continue;
        }

        result = SCardTransmit(hCard, SCARD_PCI_T1, sendData.data(), sendData.size(), nullptr, recvBuffer.data(), &recvLength);
    }

    if (result != SCARD_S_SUCCESS) {
        throw std::runtime_error("Failed to transmit data to smart card. (result: " + std::to_string(result) + ")");
    }

    uint8_t sw1 = recvBuffer[recvLength - 2];
    uint8_t sw2 = recvBuffer[recvLength - 1];

    std::vector<uint8_t> data(recvBuffer.begin(), recvBuffer.begin() + recvLength - 2);
    return ApduResponse(sw1, sw2, data);
}

void SmartCard::disconnect() {
    if (hCard != NULL) {
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
        hCard = NULL;
    }
}

void SmartCard::release()
{
    disconnect();

    if (hContext != NULL) {
		SCardReleaseContext(hContext);
        hContext = NULL;
    }
}

}