#include "mpuDataProcessorFactory.h"
#include "videoMpuDataProcessor.h"
#include "audioMpuDataProcessor.h"
#include "subtitleMpuDataProcessor.h"

namespace MmtTlv {

std::shared_ptr<MpuDataProcessorBase> MpuDataProcessorFactory::create(uint32_t tag)
{
	switch (tag) {
	case VideoMpuDataProcessor::kAssetType:
		return std::make_shared<VideoMpuDataProcessor>();
	case AudioMpuDataProcessor::kAssetType:
		return std::make_shared<AudioMpuDataProcessor>();
	case SubtitleMpuDataProcessor::kAssetType:
		return std::make_shared<SubtitleMpuDataProcessor>();
	}

	return {};
}

}