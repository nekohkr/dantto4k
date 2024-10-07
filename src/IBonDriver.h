#pragma once

class IBonDriver
{
public:
	virtual const bool OpenTuner(void) = 0;
	virtual void CloseTuner(void) = 0;

	virtual const bool SetChannel(const uint8_t bCh) = 0;
	virtual const float GetSignalLevel(void) = 0;

	virtual const uint32_t WaitTsStream(const uint32_t dwTimeOut = 0) = 0;
	virtual const uint32_t GetReadyCount(void) = 0;

	virtual const bool GetTsStream(uint8_t* pDst, uint32_t* pdwSize, uint32_t* pdwRemain) = 0;
	virtual const bool GetTsStream(uint8_t** ppDst, uint32_t* pdwSize, uint32_t* pdwRemain) = 0;

	virtual void PurgeTsStream(void) = 0;

	virtual void Release(void) = 0;
};
