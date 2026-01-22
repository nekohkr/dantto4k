#include "mpuProcessorFactory.h"
#include "mpuApplicationProcessor.h"
#include "mpuVideoProcessor.h"
#include "mpuAudioProcessor.h"
#include "mpuSubtitleProcessor.h"

namespace MmtTlv {

std::unique_ptr<MpuProcessorBase> MpuProcessorFactory::create(uint32_t tag) {
	switch (tag) {
	case MpuVideoProcessor::kAssetType:
		return std::make_unique<MpuVideoProcessor>();
	case MpuAudioProcessor::kAssetType:
		return std::make_unique<MpuAudioProcessor>();
	case MpuSubtitleProcessor::kAssetType:
		return std::make_unique<MpuSubtitleProcessor>();
	case MpuApplicationProcessor::kAssetType:
		return std::make_unique<MpuApplicationProcessor>();
	}

	return nullptr;
}

}