#include "ntp.h"

bool MmtTlv::NTPv4::unpack(Common::ReadStream& stream)
{
	uint8_t uint8 = stream.get8U();
	leap_indicator = uint8 & 0b11000000;
	version_number = uint8 & 0b00111000;
	mode = uint8 & 0b00000111;

	stratum = stream.get8U();
	poll_interval = stream.get8U();
	precision = stream.get8U();

	root_delay = stream.getBe32U();
	root_dispersion = stream.getBe32U();
	reference_id = stream.getBe32U();

	if (!reference_timestamp.unpack(stream)) {
		return false;
	}
	if (!origin_timestamp.unpack(stream)) {
		return false;
	}
	if (!receive_timestamp.unpack(stream)) {
		return false;
	}
	if (!transmit_timestamp.unpack(stream)) {
		return false;
	}

	return true;
}


bool MmtTlv::NtpTimestamp::unpack(Common::ReadStream& stream)
{
	try {
		seconds = stream.getBe32U();
		fraction = stream.getBe32U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}
