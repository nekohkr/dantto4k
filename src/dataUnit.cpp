#include "dataUnit.h"

namespace MmtTlv {

bool DataUnit::unpack(Common::ReadStream& stream, bool timedFlag, bool aggregateFlag) {
	try {
		if (timedFlag) {
			if (aggregateFlag == 0) {
				movieFragmentSequenceNumber = stream.getBe32U();
				sampleNumber = stream.getBe32U();
				offset = stream.getBe32U();
				priority = stream.get8U();
				dependencyCounter = stream.get8U();

				data.resize(stream.leftBytes());
				stream.read(data.data(), stream.leftBytes());
			}
			else {
				dataUnitLength = stream.getBe16U();
				dataUnitLength = std::min(dataUnitLength, static_cast<uint16_t>(stream.leftBytes()));

				movieFragmentSequenceNumber = stream.getBe32U();
				sampleNumber = stream.getBe32U();
				offset = stream.getBe32U();
				priority = stream.get8U();
				dependencyCounter = stream.get8U();

				if (dataUnitLength < 4 * 3 + 2) {
					return false;
				}

				data.resize(dataUnitLength - 4 * 3 - 2);
				stream.read(data.data(), dataUnitLength - 4 * 3 - 2);
			}
		}
		else {
			if (aggregateFlag == 0) {
				itemId = stream.getBe32U();

				data.resize(stream.leftBytes());
				stream.read(data.data(), stream.leftBytes());
			}
			else {
				dataUnitLength = stream.getBe16U();

				data.resize(dataUnitLength);
				stream.read(data.data(), dataUnitLength);
			}
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}