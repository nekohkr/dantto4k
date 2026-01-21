#pragma once
#include <vector>
#include <list>

class DsmccMessageHeader {
public:
    bool pack(std::vector<uint8_t>& output);
    void setProtocolDiscriminator(uint8_t protocolDiscriminator) { this->protocolDiscriminator = protocolDiscriminator; }
    void setDsmccType(uint8_t dsmccType) { this->dsmccType = dsmccType; }
    void setMessageId(uint8_t messageId) { this->messageId = messageId; }
    void setTransactionId(uint8_t transactionId) { this->transactionId = transactionId; }
    void setMessageLength(uint8_t messageLength) { this->messageLength = messageLength; }

    uint8_t protocolDiscriminator;
    uint8_t dsmccType;
    uint8_t messageId;
    uint8_t transactionId;
    uint8_t messageLength;

};

class DII {
public:
    bool pack(std::vector<uint8_t>& output);

    DsmccMessageHeader dsmccMessageHeader;
    uint8_t downloadId;
    uint8_t blockSize;
    uint8_t windowSize;
    uint8_t ackPeriod;
    uint8_t tCDownloadWindow;
    uint8_t tCDownloadScenario;
    uint8_t compatibilityDescriptor;

    class Module {
    public:
        bool pack(std::vector<uint8_t>& output);
        uint8_t moduleId;
        uint8_t moduleSize;
        uint8_t moduleVersion;
        uint8_t moduleInfoLength;
        std::vector<uint8_t> moduleInfoByte;
    };
    std::list<Module> modules;

};

class Dsmcc {
public:
	bool pack(std::vector<uint8_t>& output);
	void setTableId(uint8_t tableId) { this->tableId = tableId; }
    void setSectionSyntaxIndicator(bool sectionSyntaxIndicator) { this->sectionSyntaxIndicator = sectionSyntaxIndicator; }
    void setPrivateIndicator(bool privateIndicator) { this->privateIndicator = privateIndicator; }
    void setVersionNumber(uint8_t versionNumber) { this->versionNumber = versionNumber; }
    void setSectionNumber(uint8_t sectionNumber) { this->sectionNumber = sectionNumber; }
    void setLastSectionNumber(uint8_t lastSectionNumber) { this->lastSectionNumber = lastSectionNumber; }

    uint8_t tableId{ 0 };
    bool sectionSyntaxIndicator;
    bool privateIndicator;
    uint8_t versionNumber;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    DII* pDii{ nullptr };

};