#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <winscard.h>
#include "casProxyClient.h"

class ApduResponse {
public:
    ApduResponse() {}
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

class ISmartCard {
public:
    virtual ~ISmartCard() = default;
    virtual bool init() = 0;
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool isInited() const = 0;
    virtual std::vector<std::string> getReaders() const = 0;
    virtual uint32_t transmit(const std::vector<uint8_t>& message, ApduResponse& response) = 0;
    virtual void setSmartCardReaderName(const std::string& name) = 0;
    virtual std::string getSmartCardReaderName() const = 0;

public:
    class Transaction {
    public:
        explicit Transaction(ISmartCard& sc) : sc(sc) { sc.beginTransaction(); }
        ~Transaction() { try { sc.endTransaction(); } catch (const std::runtime_error&) {} }
    private:
        ISmartCard& sc;
    };

    [[nodiscard]] std::unique_ptr<Transaction> scopedTransaction() { return std::make_unique<Transaction>(*this); }

protected:
    virtual void beginTransaction() = 0;
    virtual void endTransaction() = 0;
};

class LocalSmartCard : public ISmartCard {
public:
    LocalSmartCard();
    ~LocalSmartCard();
    bool init() override;
    void connect() override;
    void disconnect() override;
    bool isConnected() const override;
    bool isInited() const override;
    virtual std::vector<std::string> getReaders() const;
    uint32_t transmit(const std::vector<uint8_t>& message, ApduResponse& response) override;
    virtual void setSmartCardReaderName(const std::string& name);
    virtual std::string getSmartCardReaderName() const;

protected:
    void beginTransaction() override;
    void endTransaction() override;

private:
#ifdef WIN32
    HMODULE hWinSCard = nullptr;

    typedef LONG(WINAPI* FnSCardEstablishContext)(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext);
    typedef LONG(WINAPI* FnSCardReleaseContext)(SCARDCONTEXT hContext);
    typedef LONG(WINAPI* FnSCardConnect)(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol);
    typedef LONG(WINAPI* FnSCardDisconnect)(SCARDHANDLE hCard, DWORD dwDisposition);
    typedef LONG(WINAPI* FnSCardBeginTransaction)(SCARDHANDLE hCard);
    typedef LONG(WINAPI* FnSCardEndTransaction)(SCARDHANDLE hCard, DWORD dwDisposition);
    typedef LONG(WINAPI* FnSCardTransmit)(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);
    typedef LONG(WINAPI* FnSCardListReaders)(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders);
    typedef LONG(WINAPI* FnSCardFreeMemory)(SCARDCONTEXT hContext, LPCVOID pvMem);

    FnSCardEstablishContext pSCardEstablishContext = nullptr;
    FnSCardReleaseContext pSCardReleaseContext = nullptr;
    FnSCardConnect pSCardConnect = nullptr;
    FnSCardDisconnect pSCardDisconnect = nullptr;
    FnSCardBeginTransaction pSCardBeginTransaction = nullptr;
    FnSCardEndTransaction pSCardEndTransaction = nullptr;
    FnSCardTransmit pSCardTransmit = nullptr;
    FnSCardListReaders pSCardListReaders = nullptr;
    FnSCardFreeMemory pSCardFreeMemory = nullptr;
#endif

    SCARDCONTEXT hContext = 0;
    SCARDHANDLE hCard = 0;
    DWORD dwActiveProtocol = 0;
    std::string smartCardReaderName;

};

class RemoteSmartCard : public ISmartCard {
public:
    RemoteSmartCard(std::string casProxyHost, uint16_t casProxyPort);
    ~RemoteSmartCard();
    bool init() override;
    void connect() override;
    void disconnect() override;
    bool isConnected() const override;
    bool isInited() const;
    virtual std::vector<std::string> getReaders() const;
    uint32_t transmit(const std::vector<uint8_t>& message, ApduResponse& response) override;
    virtual void setSmartCardReaderName(const std::string& name);
    virtual std::string getSmartCardReaderName() const;

protected:
    void beginTransaction() override;
    void endTransaction() override;

private:
    SCARDCONTEXT hContext = 0;
    SCARDHANDLE hCard = 0;
    DWORD dwActiveProtocol = 0;
    std::string smartCardReaderName;
    std::unique_ptr<CasProxyClient> client;


};
