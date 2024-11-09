#include "mmtStream.h"
#include "videoComponentDescriptor.h"
#include "mhAudioComponentDescriptor.h"

namespace MmtTlv {

namespace {

enum AVRounding {
    AV_ROUND_ZERO = 0,
    AV_ROUND_INF = 1,
    AV_ROUND_DOWN = 2,
    AV_ROUND_UP = 3,
    AV_ROUND_NEAR_INF = 5,
};

int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding rnd) {
    int64_t r = 0;

    if (a < 0 && a != INT64_MIN) return -av_rescale_rnd(-a, b, c, (enum AVRounding)(rnd ^ ((rnd >> 1) & 1)));

    if (rnd == AV_ROUND_NEAR_INF) r = c / 2;
    else if (rnd & 1)             r = c - 1;

    if (b <= INT_MAX && c <= INT_MAX) {
        if (a <= INT_MAX)
            return (a * b + r) / c;
        else
            return a / c * b + (a % c * b + r) / c;
    }
    else {
        uint64_t a0 = a & 0xFFFFFFFF;
        uint64_t a1 = a >> 32;
        uint64_t b0 = b & 0xFFFFFFFF;
        uint64_t b1 = b >> 32;
        uint64_t t1 = a0 * b1 + a1 * b0;
        uint64_t t1a = t1 << 32;
        int i;

        a0 = a0 * b0 + t1a;
        a1 = a1 * b1 + (t1 >> 32) + (a0 < t1a);
        a0 += r;
        a1 += a0 < r;

        for (i = 63; i >= 0; i--) {
            a1 += a1 + ((a0 >> i) & 1);
            t1 += t1;
            if (c <= a1) {
                a1 -= c;
                t1++;
            }
        }
        return t1;
    }
}

int64_t av_rescale(int64_t a, int64_t b, int64_t c) {
    return av_rescale_rnd(a, b, c, AV_ROUND_NEAR_INF);
}

} // anonymous namespace

std::pair<int64_t, int64_t> MmtStream::getNextPtsDts()
{
    const auto timestamp = getCurrentTimestamp();

    int64_t ptime = av_rescale(timestamp.first.mpuPresentationTime, timeBase.den,
        1000000ll * timeBase.num);

    if (auIndex >= timestamp.second.numOfAu) {
        throw std::out_of_range("au index out of bounds");
    }

    int64_t dts = ptime - timestamp.second.mpuDecodingTimeOffset;
    for (int j = 0; j < auIndex; ++j) {
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

uint32_t MmtStream::getSamplingRate() const
{
    if (!mhAudioComponentDescriptor) {
        return 0;
    }

    return mhAudioComponentDescriptor->getConvertedSamplingRate();
}

}