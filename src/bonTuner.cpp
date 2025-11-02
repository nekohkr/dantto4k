#include "bonTuner.h"
#include <iostream>
#include <mutex>
#include "config.h"
#include "bonDriverContext.h"

namespace {

std::vector<uint8_t> inputBuffer;
std::vector<uint8_t> outputBuffer;

}

bool CBonTuner::init() {
	HINSTANCE hBonDriverDLL = LoadLibraryA(config.bondriverPath.c_str());
	if (!hBonDriverDLL) {
		std::cerr << "Failed to load BonDriver: " << std::showbase << std::hex << GetLastError() << std::endl;
		return false;
	}

	IBonDriver2* (*CreateBonDriver)();
	CreateBonDriver = (IBonDriver2 * (*)())GetProcAddress(hBonDriverDLL, "CreateBonDriver");

	if (!CreateBonDriver) {
		FreeLibrary(hBonDriverDLL);
		hBonDriverDLL = NULL;

		std::cerr << "Failed to get address CreateBonDriver()" << std::endl;
		return false;
	}
	
	pBonDriver2 = CreateBonDriver();

	if (!pBonDriver2) {
		FreeLibrary(hBonDriverDLL);
		hBonDriverDLL = NULL;

		std::cerr << "Failed to get IBonDriver" << std::endl;
		return false;
	}

	return true;
}

const bool CBonTuner::OpenTuner(void) {
	return pBonDriver2->OpenTuner();
}

void CBonTuner::CloseTuner(void) {
	pBonDriver2->CloseTuner();
}

const bool CBonTuner::SetChannel(const uint8_t bCh) {
	return false;
}

const float CBonTuner::GetSignalLevel(void) {
	return pBonDriver2->GetSignalLevel();
}

const uint32_t CBonTuner::WaitTsStream(const uint32_t dwTimeOut) {
	return pBonDriver2->WaitTsStream(dwTimeOut);
}

const uint32_t CBonTuner::GetReadyCount(void) {
	std::lock_guard<std::mutex> lock(mutex);
	return pBonDriver2->GetReadyCount();
}

const bool CBonTuner::GetTsStream(uint8_t* pDst, uint32_t* pdwSize, uint32_t* pdwRemain) {
	uint8_t* pSrc = nullptr;
	bool ret = GetTsStream(&pSrc, pdwSize, pdwRemain);;
	if (*pdwSize) {
		memcpy(pDst, pSrc, *pdwSize);
	}

	return ret;
}

const bool CBonTuner::GetTsStream(uint8_t** ppDst, uint32_t* pdwSize, uint32_t* pdwRemain) {
	std::lock_guard<std::mutex> lock(mutex);
	
	bool ret;
	do {
		ret = pBonDriver2->GetTsStream(ppDst, pdwSize, pdwRemain);
		if (ret) {
			if (g_bonDriverContext.mmtsDumpFs) {
                g_bonDriverContext.mmtsDumpFs->write((char*)*ppDst, *pdwSize);
			}
		
			inputBuffer.insert(inputBuffer.end(), *ppDst, *ppDst + *pdwSize);
		}
	} while(ret && *pdwRemain != 0);
	
	MmtTlv::Common::ReadStream input(inputBuffer);
	while (!input.isEof()) {
		MmtTlv::DemuxStatus status = g_bonDriverContext.demuxer.demux(input);

		if (status == MmtTlv::DemuxStatus::NotEnoughBuffer) {
			break;
		}
	}

	inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + (inputBuffer.size() - input.leftBytes()));

	if (g_bonDriverContext.remuxOutput.size() < 188 * 1024) {
		return false;
	}

	outputBuffer = std::move(g_bonDriverContext.remuxOutput);

	*ppDst = outputBuffer.data();
	*pdwSize = static_cast<uint32_t>(outputBuffer.size());
	*pdwRemain = 0;
	return true;
}

void CBonTuner::PurgeTsStream(void) {
	std::lock_guard<std::mutex> lock(mutex);

	inputBuffer.clear();
	g_bonDriverContext.remuxOutput.clear();
	g_bonDriverContext.demuxer.clear();

	return pBonDriver2->PurgeTsStream();
}

const char* CBonTuner::GetTunerName(void) {
	return pBonDriver2->GetTunerName();
}

const bool CBonTuner::IsTunerOpening(void) {
	return pBonDriver2->IsTunerOpening();
}

const char* CBonTuner::EnumTuningSpace(const uint32_t dwSpace) {
	return pBonDriver2->EnumTuningSpace(dwSpace);
}

const char* CBonTuner::EnumChannelName(const uint32_t dwSpace, const uint32_t dwChannel) {
	return pBonDriver2->EnumChannelName(dwSpace, dwChannel);
}

const bool CBonTuner::SetChannel(const uint32_t dwSpace, const uint32_t dwChannel) {
	std::lock_guard<std::mutex> lock(mutex);

	inputBuffer.clear();
	g_bonDriverContext.remuxOutput.clear();
	g_bonDriverContext.demuxer.clear();

	if (config.mmtsDumpPath != "") {
		if (g_bonDriverContext.mmtsDumpFs) {
            g_bonDriverContext.mmtsDumpFs->close();
            g_bonDriverContext.mmtsDumpFs.reset();
		}

		g_bonDriverContext.mmtsDumpFs = std::make_unique<std::ofstream>(config.mmtsDumpPath, std::ios::binary);
	}

	return pBonDriver2->SetChannel(dwSpace, dwChannel);
}

const uint32_t CBonTuner::GetCurSpace(void) {
	return pBonDriver2->GetCurSpace();
}

const uint32_t CBonTuner::GetCurChannel(void) {
	return pBonDriver2->GetCurChannel();
}

void CBonTuner::Release(void) {
	return pBonDriver2->Release();
}
