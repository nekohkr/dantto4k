#include "mpuDataProcessorFactory.h"
#include "hevcMpuDataProcessor.h"
#include "aacMpuDataProcessor.h"
#include "ttmlMpuDataProcessor.h"

std::shared_ptr<MpuDataProcessorBase> MpuDataProcessorFactory::create(uint32_t tag)
{
	switch (tag) {
	case HevcMpuDataProcessor::kAssetType:
		return std::make_shared<HevcMpuDataProcessor>();
	case AacMpuDataProcessor::kAssetType:
		return std::make_shared<AacMpuDataProcessor>();
	case TtmlMpuDataProcessor::kAssetType:
		return std::make_shared<TtmlMpuDataProcessor>();
	}

	return {};
}