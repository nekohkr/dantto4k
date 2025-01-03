#include "bonTuner.h"
#include <iostream>
#include <mutex>
#include "dantto4k.h"
#include "config.h"

std::vector<uint8_t> inputBuffer;
std::vector<uint8_t> outputBuffer;
FILE* fp = nullptr;

bool CBonTuner::init()
{
	HINSTANCE hBonDriverDLL = LoadLibraryA(config.bondriverPath.c_str());
	if (!hBonDriverDLL) {
		std::cerr << "Failed to load BonDriver (Error code: " << GetLastError() << ")" << std::endl;
		return false;
	}

	IBonDriver2* (*CreateBonDriver)();
	CreateBonDriver = (IBonDriver2 * (*)())GetProcAddress(hBonDriverDLL, "CreateBonDriver");

	if (!CreateBonDriver) {
		FreeLibrary(hBonDriverDLL);
		hBonDriverDLL = NULL;

		std::cerr << "Could not get address CreateBonDriver()" << std::endl;
		return false;
	}
	
	pBonDriver2 = CreateBonDriver();

	if (!pBonDriver2) {
		FreeLibrary(hBonDriverDLL);
		hBonDriverDLL = NULL;

		std::cerr << "Could not get IBonDriver" << std::endl;
		return false;
	}

	return true;
}

const bool CBonTuner::OpenTuner(void)
{
	return pBonDriver2->OpenTuner();
}

void CBonTuner::CloseTuner(void)
{
	pBonDriver2->CloseTuner();
}

const bool CBonTuner::SetChannel(const uint8_t bCh)
{
	return false;
}

const float CBonTuner::GetSignalLevel(void)
{
	return pBonDriver2->GetSignalLevel();
}

const uint32_t CBonTuner::WaitTsStream(const uint32_t dwTimeOut)
{
	return pBonDriver2->WaitTsStream(dwTimeOut);
}

const uint32_t CBonTuner::GetReadyCount(void)
{
	std::lock_guard<std::mutex> lock(mutex);
	return pBonDriver2->GetReadyCount();
}

const bool CBonTuner::GetTsStream(uint8_t* pDst, uint32_t* pdwSize, uint32_t* pdwRemain)
{
	uint8_t* pSrc = nullptr;
	bool ret = GetTsStream(&pSrc, pdwSize, pdwRemain);;
	if (*pdwSize) {
		memcpy(pDst, pSrc, *pdwSize);
	}

	return ret;
}

#ifdef _WIN32
static int processPacketWithHandler(MmtTlv::Common::ReadStream& input) {
	__try {
		return demuxer.processPacket(input);
	}
	__except (ExceptionHandler(GetExceptionInformation())) {
		inputBuffer.clear();
	}

	return 0;
}
#endif

const bool CBonTuner::GetTsStream(uint8_t** ppDst, uint32_t* pdwSize, uint32_t* pdwRemain)
{
	std::lock_guard<std::mutex> lock(mutex);
	
	bool ret;
	do {
		ret = pBonDriver2->GetTsStream(ppDst, pdwSize, pdwRemain);
		if (ret) {
			if (fp) {
				fwrite(*ppDst, 1, *pdwSize, fp);
			}
		
			inputBuffer.insert(inputBuffer.end(), *ppDst, *ppDst + *pdwSize);
		}
	} while(ret && *pdwRemain != 0);
	
	MmtTlv::Common::ReadStream input(inputBuffer);
	while (!input.isEof()) {
		size_t cur = input.getCur();
		int n = 0;
		
		#ifdef _WIN32
		n = processPacketWithHandler(input);
		#else
		n = demuxer.processPacket(input);
		#endif

		// not valid tlv
		if (n == -2) {
			continue;
		}

		// not enough buffer for tlv payload
		if (n == -1) {
			input.setCur(cur);
			break;
		}
	}

	inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + (inputBuffer.size() - input.leftBytes()));

	if (output.size() < 188 * 1024) {
		return false;
	}

	outputBuffer = std::move(output);

	*ppDst = outputBuffer.data();
	*pdwSize = static_cast<uint32_t>(outputBuffer.size());
	*pdwRemain = 0;
	return true;
}

void CBonTuner::PurgeTsStream(void)
{
	std::lock_guard<std::mutex> lock(mutex);

	inputBuffer.clear();
	output.clear();
	
	demuxer.clear();

	return pBonDriver2->PurgeTsStream();
}

const char* CBonTuner::GetTunerName(void)
{
	return pBonDriver2->GetTunerName();
}

const bool CBonTuner::IsTunerOpening(void)
{
	return pBonDriver2->IsTunerOpening();
}

const char* CBonTuner::EnumTuningSpace(const uint32_t dwSpace)
{
	return pBonDriver2->EnumTuningSpace(dwSpace);
}

const char* CBonTuner::EnumChannelName(const uint32_t dwSpace, const uint32_t dwChannel)
{
	return pBonDriver2->EnumChannelName(dwSpace, dwChannel);
}

const bool CBonTuner::SetChannel(const uint32_t dwSpace, const uint32_t dwChannel)
{
	std::lock_guard<std::mutex> lock(mutex);

	inputBuffer.clear();
	output.clear();

	if (config.mmtsDumpPath != "") {
		if (fp) {
			fclose(fp);
		}

		fp = fopen(config.mmtsDumpPath.c_str(), "wb");
	}

	demuxer.clear();

	return pBonDriver2->SetChannel(dwSpace, dwChannel);
}

const uint32_t CBonTuner::GetCurSpace(void)
{
	return pBonDriver2->GetCurSpace();
}

const uint32_t CBonTuner::GetCurChannel(void)
{
	return pBonDriver2->GetCurChannel();
}

void CBonTuner::Release(void)
{
	demuxer.release();
	return pBonDriver2->Release();
}
