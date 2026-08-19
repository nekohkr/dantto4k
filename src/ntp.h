#pragma once
#include "stream.h"
#include "ntpTimestamp.h"

namespace MmtTlv {

class NTPv4 {
public:
	bool unpack(Common::ReadStream& stream);

public:
	uint8_t leap_indicator;
	uint8_t version_number;
	uint8_t mode;

	uint8_t stratum;
	uint8_t poll_interval;
	uint8_t precision;

	uint32_t root_delay;
	uint32_t root_dispersion;
	uint32_t reference_id;

	NtpTimestamp reference_timestamp;
	NtpTimestamp origin_timestamp;
	NtpTimestamp receive_timestamp;
	NtpTimestamp transmit_timestamp;

};

}
