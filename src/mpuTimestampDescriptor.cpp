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

bool MpuTimestampDescriptor::unpack(Stream& stream)
{
	try {
		if (!MmtDescriptor::unpack(stream)) {
			return false;
		}

		for (int i = 0; i < descriptorLength / 12; i++) {
			Entry entry;
			entry.mpuSequenceNumber = stream.getBe32U();
			entry.mpuPresentationTime = ff_parse_ntp_time2(stream.getBe64U()) - NTP_OFFSET_US;
			entries.push_back(entry);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}
