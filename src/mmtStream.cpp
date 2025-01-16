#include "mmtStream.h"
#include "videoComponentDescriptor.h"
#include "mhAudioComponentDescriptor.h"
#include "timebase.h"

namespace MmtTlv {

std::pair<int64_t, int64_t> MmtStream::getNextPtsDts()
{
    const auto timestamp = getCurrentTimestamp();

    int64_t ptime = av_rescale(timestamp.first.mpuPresentationTime, timeBase.den,
        1000000ll * timeBase.num);

    if (auIndex >= timestamp.second.numOfAu) {
        throw std::out_of_range("au index out of bounds");
    }

    int64_t dts = ptime - timestamp.second.mpuDecodingTimeOffset;
    for (uint32_t j = 0; j < auIndex; ++j) {
        dts += timestamp.second.ptsOffsets[j];
    }

    int64_t pts = dts + timestamp.second.dtsPtsOffsets[auIndex];

    ++auIndex;

    return std::pair<int64_t, int64_t>(pts, dts);
}

std::pair<const MpuTimestampDescriptor::Entry, const MpuExtendedTimestampDescriptor::Entry> MmtStream::getCurrentTimestamp() const
{
    int mpuTimestampIndex = -1;
    int extendedTimestampIndex = -1;

    for (int i = 0; i < mpuTimestamps.size(); ++i) {
        if (mpuTimestamps[i].mpuSequenceNumber ==
            lastMpuSequenceNumber) {
            mpuTimestampIndex = i;
            break;
        }
    }

    for (int i = 0; i < mpuExtendedTimestamps.size(); ++i) {
        if (mpuExtendedTimestamps[i].mpuSequenceNumber ==
            lastMpuSequenceNumber) {
            extendedTimestampIndex = i;
            break;
        }
    }

    if (mpuTimestampIndex == -1 || extendedTimestampIndex == -1) {
        return {};
    }

    return { mpuTimestamps[mpuTimestampIndex], mpuExtendedTimestamps[extendedTimestampIndex] };
}


bool MmtStream::Is8KVideo() const
{
    if (!videoComponentDescriptor) {
        return false;
    }

    return videoComponentDescriptor->Is8KVideo();
}

bool MmtStream::Is22_2chAudio() const
{
    if (!mhAudioComponentDescriptor) {
        return false;
    }

    return mhAudioComponentDescriptor->Is22_2chAudio();
}

uint32_t MmtStream::getSamplingRate() const
{
    if (!mhAudioComponentDescriptor) {
        return 0;
    }

    return mhAudioComponentDescriptor->getConvertedSamplingRate();
}

}