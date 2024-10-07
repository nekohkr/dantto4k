#pragma once
#include <stdint.h>
#include "IBonDriver.h"

class IBonDriver2 : public IBonDriver
{
public:
	virtual const char* GetTunerName(void) = 0;

	virtual const bool IsTunerOpening(void) = 0;
	virtual const char* EnumTuningSpace(const uint32_t dwSpace) = 0;
	virtual const char* EnumChannelName(const uint32_t dwSpace, const uint32_t dwChannel) = 0;

	virtual const bool SetChannel(const uint32_t dwSpace, const uint32_t dwChannel) = 0;

	virtual const uint32_t GetCurSpace(void) = 0;
	virtual const uint32_t GetCurChannel(void) = 0;

	// IBonDriver
	virtual void Release(void) = 0;
};