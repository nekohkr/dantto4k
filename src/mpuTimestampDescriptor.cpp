#include "mpuTimestampDescriptor.h"

#define 	NTP_OFFSET   2208988800ULL
#define 	NTP_OFFSET_US   (NTP_OFFSET * 1000000ULL)

static uint64_t ff_parse_ntp_time2(uint64_t ntp_ts)
{
	uint64_t sec = ntp_ts >> 32;
	uint64_t frac_part = ntp_ts & 0xFFFFFFFFULL;
	uint64_t usec = (frac_part * 1000000) / 0xFFFFFFFFULL;

	return (sec * 1000000) + usec;
}

bool NTPTimestamp::unpack(Stream& stream)
{
	ntp = ff_parse_ntp_time2(stream.getBe64U()) - NTP_OFFSET_US;
	return true;
}

bool MpuTimestampDescriptor::unpack(Stream& stream)
{
	if (stream.leftBytes() < 3) {
		return false;
	}

	if (!MmtDescriptor::unpack(stream)) {
		return false;
	}

	for (int i = 0; i < descriptorLength / 12; i++) {
		MpuTimestamp mpuTimeStamp;
		mpuTimeStamp.mpuSequenceNumber = stream.getBe32U();

		NTPTimestamp ntpTimestamp;
		ntpTimestamp.unpack(stream);
		mpuTimeStamp.mpuPresentationTime = ntpTimestamp.ntp;
		mpuTimestamps.push_back(mpuTimeStamp);
	}
	return true;
}
