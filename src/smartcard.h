#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
#define _WINSOCKAPI_
#include <Windows.h>
#endif
#include <winscard.h>

namespace MmtTlv::Acas {

class ApduResponse {
public:
    ApduResponse(uint8_t sw1, uint8_t sw2, const std::vector<uint8_t>& data = {})
        : sw1(sw1), sw2(sw2), data(data) {}

    uint8_t getSw1() const {
        return sw1;
    }

    uint8_t getSw2() const {
        return sw2;
    }

    const std::vector<uint8_t>& getData() const {
        return data;
    }

    bool isSuccess() const {
        return sw1 == 0x90 && sw2 == 0x00;
    }

private:
    uint8_t sw1;
    uint8_t sw2;
    std::vector<uint8_t> data;
};

class ApduCommand {
public:
    ApduCommand(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2)
        : cla(cla), ins(ins), p1(p1), p2(p2) {}

    std::vector<uint8_t> case1() {
        return { cla, ins, p1, p2 };
    }

    std::vector<uint8_t> case2short(uint8_t le) {
        return { cla, ins, p1, p2, le };
    }

    std::vector<uint8_t> case3short(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> apdu = { cla, ins, p1, p2, static_cast<uint8_t>(data.size()) };
        apdu.insert(apdu.end(), data.begin(), data.end());
        return apdu;
    }

    std::vector<uint8_t> case4short(const std::vector<uint8_t>& data, uint8_t le) {
        std::vector<uint8_t> apdu = { cla, ins, p1, p2, static_cast<uint8_t>(data.size()) };
        apdu.insert(apdu.end(), data.begin(), data.end());
        apdu.push_back(le);
        return apdu;
    }

private:
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
};

class SmartCard {
public:
    void setSmartCardReaderName(const std::string& smartCardReaderName) {
        this->smartCardReaderName = smartCardReaderName;
    }
    bool init();
    bool isConnected() const;
    void connect();
    ApduResponse transmit(const std::vector<BYTE>& sendData);
    void disconnect();
    void release();

private:
    SCARDCONTEXT hContext = 0;
    SCARDHANDLE hCard = 0;
    DWORD dwActiveProtocol = 0;
    std::string smartCardReaderName;
};

}