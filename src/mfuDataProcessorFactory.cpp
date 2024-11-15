#include "mfuDataProcessorFactory.h"
#include "applicationMfuDataProcessor.h"
#include "videoMfuDataProcessor.h"
#include "audioMfuDataProcessor.h"
#include "subtitleMfuDataProcessor.h"

namespace MmtTlv {

std::shared_ptr<MfuDataProcessorBase> MfuDataProcessorFactory::create(uint32_t tag)
{
	switch (tag) {
	case VideoMfuDataProcessor::kAssetType:
		return std::make_shared<VideoMfuDataProcessor>();
	case AudioMfuDataProcessor::kAssetType:
		return std::make_shared<AudioMfuDataProcessor>();
	case SubtitleMfuDataProcessor::kAssetType:
		return std::make_shared<SubtitleMfuDataProcessor>();
	case ApplicationMfuDataProcessor::kAssetType:
		return std::make_shared<ApplicationMfuDataProcessor>();
	}

	return {};
}

}