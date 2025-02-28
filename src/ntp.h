#pragma once
#include "stream.h"
#include "timebase.h"

namespace MmtTlv {

	class NtpTimestamp {
	public:
		bool unpack(Common::ReadStream& stream);
		int64_t toPcrValue() const {
			const uint32_t NTP_1970 = 2208988800U;
			int64_t unixTimestamp = static_cast<int64_t>(((seconds - NTP_1970) * 1000.0) + ((fraction / 4294967296.0) * 1000.0));

			const AVRational ntpTimeBase = { 1, 1000 };
			const AVRational pcrTimeBase = { 1, 27000000 };

			return av_rescale_q(unixTimestamp, ntpTimeBase, pcrTimeBase);
		}

	public:
		uint32_t seconds;
		uint32_t fraction;

	};

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