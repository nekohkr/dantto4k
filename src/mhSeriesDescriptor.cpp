#include "mhSeriesDescriptor.h"

bool MhSeriesDescriptor::unpack(Stream& stream)
{
    try {
        if (!MmtDescriptor::unpack(stream)) {
            return false;
        }

        Stream nstream(stream, descriptorLength);

        seriesId = nstream.getBe16U();

        uint8_t uint8 = nstream.get8U();

        repeatLabel = (uint8 & 0b11110000) >> 4;
        programPattern = (uint8 & 0b00001110) >> 1;
        expireDateValidFlag = uint8 & 1;

        expireDate = nstream.getBe16U();

        uint8_t uint16 = nstream.getBe16U();
        episodeNumber = (uint16 & 0b1111111111110000) >> 4;
        lastEpisodeNumber = (uint16 & 0b0000000000001111) << 8 | nstream.get8U();

        seriesNameChar.resize(nstream.leftBytes());
        nstream.read(seriesNameChar.data(), nstream.leftBytes());

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}