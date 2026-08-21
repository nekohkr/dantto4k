#pragma once
#include <cstdint>
#include "stream.h"

namespace MmtTlv {

class NtpTimestamp {
public:
	bool unpack(Common::ReadStream& stream);
	int64_t toPcrValue() const;
	int64_t toPtsValue() const;

	uint32_t seconds{};
	uint32_t fraction{};
};

}
