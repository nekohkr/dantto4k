#include "mhCaContractInformation.h"

namespace MmtTlv {

bool MhCaContractInformation::unpack(Common::Stream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::Stream nstream(stream, descriptorLength);

        caSystemId = nstream.getBe16U();

        uint8_t uint8 = nstream.get8U();
        caUnitId = (uint8 & 0b11110000) >> 4;
        numOfComponent = uint8 & 0b00001111;

        for (int i = 0; i < numOfComponent; i++) {
            componentTags.push_back(nstream.getBe16U());
        }

        contractVerificationInfoLength = nstream.get8U();
        contractVerificationInfo.resize(contractVerificationInfoLength);
        nstream.read(contractVerificationInfo.data(), contractVerificationInfoLength);

        feeNameLength = nstream.get8U();
        feeName.resize(feeNameLength);
        nstream.read(feeName.data(), feeNameLength);

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}