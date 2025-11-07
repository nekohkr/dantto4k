#include "casProxyClient.h"

CasProxyClient::CasProxyClient(const std::string& host, uint16_t port)
    : host(host), port(port), resolver(io_context), socket(io_context), connectionTimer(io_context), workGuard(asio::make_work_guard(io_context)) {
    thread = std::thread([this]() { io_context.run(); });
}

CasProxyClient::~CasProxyClient() {
    close();
}

void CasProxyClient::connect() {
    std::lock_guard<std::mutex> lock(connectionMutex);
    if (connecting) {
        return;
    }

    connecting = true;
    connectionTimer.expires_after(std::chrono::milliseconds(kConnectionTimeoutMs));
    connectionTimer.async_wait([&](const std::error_code& error) {
        if (!error && !connected) {
            onFail();
        }
    });

    asio::ip::tcp::resolver::query query(host, std::to_string(port));
    resolver.async_resolve(query,
        [this](std::error_code ec, asio::ip::tcp::resolver::results_type results) {
            if (!ec) {
                asio::async_connect(socket, results,
                    [this](std::error_code ec, const asio::ip::tcp::endpoint&) {
                        connectionTimer.cancel();
                        if (!ec) {
                            onOpen();
                            doRead();
                        }
                        else {
                            onFail();
                        }
                    });
            }
            else {
                connectionTimer.cancel();
                onFail();
            }
        }
    );
}

void CasProxyClient::close() {
    for (auto& promise : mapResponsePromise) {
        promise.second.set_value(nullptr);
    }
    mapResponsePromise.clear();

    std::deque<std::vector<uint8_t>> empty;
    sendQueue.swap(empty);

    if (socket.is_open()) {
        asio::error_code ec;
        socket.cancel(ec);
        socket.close(ec);
    }

    connectionTimer.cancel();

    workGuard.reset();
    io_context.stop();
    if (thread.joinable()) {
        thread.join();
    }

    std::lock_guard<std::mutex> lock(connectionMutex);
    connected = false;
    connecting = false;
}

void CasProxyClient::doRead() {
    asio::async_read(socket, asio::buffer(&packetLength, 4),
        [this](std::error_code ec, std::size_t) {
            if (!ec) {
                packetLength = casproxy::swapEndian32(packetLength);
                if (packetLength > 100 * 1024) {
                    return;
                }

                readPacketData();
            }
            else if (ec != asio::error::eof) {
                close();
            }
        }
    );
}

void CasProxyClient::readPacketData() {
    packetData.resize(packetLength);
    asio::async_read(socket, asio::buffer(packetData),
        [this](std::error_code ec, std::size_t) {
            if (!ec) {
                handleResponse();
                doRead();
            }
            else if (ec != asio::error::eof) {
                close();
            }
        }
    );
}

void CasProxyClient::handleResponse() {
    casproxy::StreamReader reader(packetData);

    uint32_t packetId, resultCode, opcodeValue;
    if (!reader.read(packetId) || !reader.read(resultCode) || !reader.read(opcodeValue)) {
        return;
    }

    packetId = casproxy::swapEndian32(packetId);
    resultCode = casproxy::swapEndian32(resultCode);
    opcodeValue = casproxy::swapEndian32(opcodeValue);
    casproxy::Opcode opcode = static_cast<casproxy::Opcode>(opcodeValue);

    auto res = casproxy::ResponseFactory::create(opcode);
    if (!res) {
        return;
    }

    if (!res->unpack(packetId, resultCode, reader)) {
        return;
    }

    std::lock_guard<std::mutex> lock(responsePromiseMutex);
    auto it = mapResponsePromise.find(packetId);
    if (it != mapResponsePromise.end()) {
        it->second.set_value(std::move(res));
    }
}

void CasProxyClient::onOpen() {
    std::lock_guard<std::mutex> lock(connectionMutex);
    connected = true;
    connecting = false;

    if (connectionPromise) {
        connectionPromise->set_value(true);
        connectionPromise = std::nullopt;
    }
}

void CasProxyClient::onFail() {
    close();

    std::lock_guard<std::mutex> lock(connectionMutex);
    connected = false;
    connecting = false;


    if (connectionPromise) {
        connectionPromise->set_value(false);
        connectionPromise = std::nullopt;
    }
}

void CasProxyClient::onClose() {
    connected = false;
}

std::optional<std::shared_ptr<casproxy::ResponseBase>> CasProxyClient::sendRequest(casproxy::RequestBase& req) {
    if (connected == false) {
        connectionPromise = std::promise<bool>();
        connect();

        auto future = connectionPromise->get_future();
        if (!future.get()) {
            throw std::runtime_error("Failed to connect to CasProxyServer");
        }
    }

    std::lock_guard<std::mutex> lock(mutex);
    casproxy::StreamWriter writer;
    req.packetId = packetId.fetch_add(1, std::memory_order_relaxed);
    req.pack(writer);

    uint32_t packetLength = static_cast<uint32_t>(writer.buffer.size());
    std::vector<uint8_t> packet(packetLength + 4);
    packetLength = casproxy::swapEndian32(packetLength);
    memcpy(packet.data(), &packetLength, 4);
    memcpy(packet.data() + 4, writer.buffer.data(), writer.buffer.size());

    std::promise<std::shared_ptr<casproxy::ResponseBase>> promise;
    auto future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(responsePromiseMutex);
        mapResponsePromise.emplace(req.packetId, std::move(promise));
    }

    sendQueue.push_back(packet);
    if (sendQueue.size() < 2) {
        doWrite();
    }

    if (future.wait_for(std::chrono::milliseconds(kRequestTimeoutMs)) == std::future_status::timeout) {
        std::lock_guard<std::mutex> lock(responsePromiseMutex);
        mapResponsePromise.erase(req.packetId);
        throw std::runtime_error("Timeout while requesting CasProxyServer");
    }

    auto res = future.get();
    {
        std::lock_guard<std::mutex> lock(responsePromiseMutex);
        mapResponsePromise.erase(req.packetId);
    }

    if (!res) {
        return std::nullopt;
    }

    return res;
}

void CasProxyClient::doWrite() {
    if (sendQueue.empty()) {
        return;
    }

    asio::async_write(socket, asio::buffer(sendQueue.front()),
        [this](std::error_code ec, std::size_t) {
            if (ec) {
                close();
            }
            else {
                sendQueue.pop_front();
                doWrite();
            }
        }
    );
}

LONG CasProxyClient::scardEstablishContext(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext) {
    casproxy::SCardEstablishContextRequest req;
    req.dwScope = dwScope;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardEstablishContextResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    if (phContext) {
        *phContext = res->hContext;
    }

    return res->apiReturn;
}

LONG CasProxyClient::scardReleaseContext(SCARDCONTEXT hContext) {
    casproxy::SCardReleaseContextRequest req;
    req.hContext = hContext;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardReleaseContextResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    return res->apiReturn;
}

LONG CasProxyClient::scardListReaders(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders) {
    if (pcchReaders == nullptr) {
        return SCARD_E_INVALID_PARAMETER;
    }

    if (*pcchReaders == SCARD_AUTOALLOCATE) {
        uint32_t readersLength = 0;
        {
            casproxy::SCardListReadersRequest req;
            req.hContext = hContext;
            if (mszGroups != nullptr) {
                req.groups = mszGroups;
            }

            req.readersLength = 0;
            auto r = sendRequest(req);
            if (!r) {
                return SCARD_F_INTERNAL_ERROR;
            }

            auto res = std::dynamic_pointer_cast<casproxy::SCardListReadersResponse>(*r);
            if (res->resultCode != 0) {
                throw std::runtime_error(std::string("CasProxyServer returned an error response"));
            }

            if (res->apiReturn != SCARD_S_SUCCESS) {
                return res->apiReturn;
            }

            if (res->readersLength == 1) {
                *(LPSTR*)mszReaders = new char[1];
                (*(LPSTR*)mszReaders)[0] = 0;
                return res->resultCode;
            }

            readersLength = res->readersLength;
        }

        {
            casproxy::SCardListReadersRequest req;
            req.hContext = hContext;
            if (mszGroups != nullptr) {
                req.groups = mszGroups;
            }
            req.readersLength = readersLength;

            auto r = sendRequest(req);
            if (!r) {
                return SCARD_F_INTERNAL_ERROR;
            }

            auto res = std::dynamic_pointer_cast<casproxy::SCardListReadersResponse>(*r);
            if (res->resultCode != 0) {
                throw std::runtime_error(std::string("CasProxyServer returned an error response"));
            }

            if (res->apiReturn != SCARD_S_SUCCESS) {
                return res->apiReturn;
            }

            if (readersLength == 0) {
                return res->resultCode;
            }

            *(LPSTR*)mszReaders = new char[res->readers.size()];
            memcpy(*(LPSTR*)mszReaders, res->readers.data(), res->readers.size());

            return res->apiReturn;
        }
    }
    else {
        casproxy::SCardListReadersRequest req;
        req.hContext = hContext;
        if (mszGroups != nullptr) {
            req.groups = mszGroups;
        }
        req.readersLength = *pcchReaders == SCARD_AUTOALLOCATE ? 1024 : *pcchReaders;

        auto r = sendRequest(req);
        if (!r) {
            return SCARD_F_INTERNAL_ERROR;
        }

        auto res = std::dynamic_pointer_cast<casproxy::SCardListReadersResponse>(*r);
        if (res->resultCode != 0) {
            throw std::runtime_error(std::string("CasProxyServer returned an error response"));
        }

        if (mszReaders != nullptr) {
            memcpy(mszReaders, res->readers.data(), std::min(*pcchReaders, static_cast<DWORD>(res->readers.size())));
        }
        *pcchReaders = res->readersLength;

        return res->apiReturn;
    }

}

LONG CasProxyClient::scardConnect(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol) {
    casproxy::SCardConnectRequest req;
    req.hContext = hContext;
    req.szReader = szReader;
    req.dwShareMode = dwShareMode;
    req.dwPreferredProtocols = dwPreferredProtocols;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardConnectResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    if (phCard != nullptr) {
        *phCard = res->hCard;
    }
    if (pdwActiveProtocol != nullptr) {
        *pdwActiveProtocol = res->dwActiveProtocol;
    }

    return res->apiReturn;
}

LONG CasProxyClient::scardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition) {
    casproxy::SCardDisconnectRequest req;
    req.hCard = hCard;
    req.dwDisposition = dwDisposition;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardDisconnectResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    return res->apiReturn;
}

LONG CasProxyClient::scardBeginTransaction(SCARDHANDLE hCard) {
    casproxy::SCardBeginTransactionRequest req;
    req.hCard = hCard;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardBeginTransactionResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    return res->apiReturn;
}

LONG CasProxyClient::scardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition) {
    casproxy::SCardEndTransactionRequest req;
    req.hCard = hCard;
    req.dwDisposition = dwDisposition;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardEndTransactionResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    return res->apiReturn;
}

LONG CasProxyClient::scardTransmit(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength) {
    casproxy::SCardTransmitRequest req;
    req.hCard = hCard;
    if (pioSendPci == nullptr) {
        req.sendPci = 3;
    }
    else {
        if (pioSendPci == SCARD_PCI_T0) {
            req.sendPci = 0;
        }
        else if (pioSendPci == SCARD_PCI_T1) {
            req.sendPci = 1;
        }
        else if (pioSendPci == SCARD_PCI_RAW) {
            req.sendPci = 2;
        }
        else {
            req.sendPci = 3;
        }
    }

    req.sendBuffer = std::vector<uint8_t>((uint8_t*)pbSendBuffer, (uint8_t*)pbSendBuffer + cbSendLength);
    if (pioRecvPci == nullptr) {
        req.isRecvPciNull = true;
    }
    else {
        req.recvPciProtocol = pioRecvPci->dwProtocol;
        req.recvPciProtocol = pioRecvPci->cbPciLength;
    }

    req.recvLength = *pcbRecvLength;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardTransmitResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    if (pbRecvBuffer != nullptr) {
        memcpy(pbRecvBuffer, res->recvBuffer.data(), std::min(*pcbRecvLength, static_cast<DWORD>(res->recvBuffer.size())));
    }

    *pcbRecvLength = res->recvLength;

    if (pioRecvPci != nullptr) {
        if (!res->isRecvPciNull) {
            pioRecvPci->dwProtocol = res->recvPciProtocol;
            pioRecvPci->cbPciLength = res->recvPciLength;
        }
    }

    return res->apiReturn;
}

LONG CasProxyClient::scardGetAttrib(SCARDHANDLE hCard, DWORD dwAttrId, LPBYTE pbAttr, LPDWORD pcbAttrLen) {
    casproxy::SCardGetAttribRequest req;
    req.hCard = hCard;
    req.dwAttrId = dwAttrId;
    req.attrLength = *pcbAttrLen;

    auto r = sendRequest(req);
    if (!r) {
        return SCARD_F_INTERNAL_ERROR;
    }

    auto res = std::dynamic_pointer_cast<casproxy::SCardGetAttribResponse>(*r);
    if (res->resultCode != 0) {
        throw std::runtime_error(std::string("CasProxyServer returned an error response"));
    }

    if (pbAttr != nullptr) {
        memcpy(pbAttr, res->attrBuffer.data(), std::min(*pcbAttrLen, static_cast<DWORD>(res->attrBuffer.size())));
    }

    *pcbAttrLen = res->attrLength;

    return res->apiReturn;
}

LONG CasProxyClient::scardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem) {
    delete[](char*)pvMem;
    return SCARD_S_SUCCESS;
}
