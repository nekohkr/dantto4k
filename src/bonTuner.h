#pragma once
#include <string>
#include "IBonDriver2.h"
#include <vector>
#include "config.h"

class CBonTuner : public IBonDriver2
{
public:
	CBonTuner();
	virtual ~CBonTuner();

	// Initialize channel
	bool init(Config& config);

	// IBonDriver
	const bool OpenTuner(void);
	void CloseTuner(void);

	const bool SetChannel(const uint8_t bCh);
	const float GetSignalLevel(void);

	const uint32_t WaitTsStream(const uint32_t dwTimeOut = 0);
	const uint32_t GetReadyCount(void);

	const bool GetTsStream(uint8_t* pDst, uint32_t* pdwSize, uint32_t* pdwRemain);
	const bool GetTsStream(uint8_t** ppDst, uint32_t* pdwSize, uint32_t* pdwRemain);

	void PurgeTsStream(void);

	// IBonDriver2(эеяв)
	const char* GetTunerName(void);

	const bool IsTunerOpening(void);

	const char* EnumTuningSpace(const uint32_t dwSpace);
	const char* EnumChannelName(const uint32_t dwSpace, const uint32_t dwChannel);

	const bool SetChannel(const uint32_t dwSpace, const uint32_t dwChannel);

	const uint32_t GetCurSpace(void);
	const uint32_t GetCurChannel(void);

	void Release(void);

protected:
	IBonDriver2* pBonDriver2;
	std::vector<uint8_t> inputBuffer;

};