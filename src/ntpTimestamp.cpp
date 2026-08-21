#include "ntpTimestamp.h"
#include "timebase.h"

namespace MmtTlv {

bool NtpTimestamp::unpack(Common::ReadStream& stream) {
	try {
		seconds = stream.getBe32U();
		fraction = stream.getBe32U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

int64_t NtpTimestamp::toPcrValue() const {
	constexpr int64_t NTP_1970 = 2208988800LL;
	const int64_t unixTimestamp = (static_cast<int64_t>(seconds) - NTP_1970) * 1000 +
		av_rescale(fraction, 1000, UINT64_C(1) << 32);
	const AVRational ntpTimeBase = { 1, 1000 };
	const AVRational pcrTimeBase = { 1, 27000000 };
	return av_rescale_q(unixTimestamp, ntpTimeBase, pcrTimeBase);
}

int64_t NtpTimestamp::toPtsValue() const {
	return toPcrValue() / 300;
}

}
