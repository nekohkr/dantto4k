#include "adtsConverter.h"
#include "stream.h"

static inline int convertAdtsAudioObjectType(int aot)
{
    switch (aot) {
    case 1:     return 0;
    case 2:     return 1;
    case 3:     return 2;
    default:    return 3;
    }
}

bool ADTSConverter::convert(uint8_t* input, int size, std::vector<uint8_t>& output)
{
	if (size < 2) {
		return false;
	}

    uint16_t syncWord = (input[0] << 3) | (input[1] & 0b11100000) >> 5;
    if (syncWord != 0x2b7) {
        return false;
    }

    uint16_t audioMuxLengthBytes = ((input[1] & 0b00011111) << 8 | input[2]) + 3;
	if (size != audioMuxLengthBytes) {
		return false;
	}

    if (!unpackStreamMuxConfig(input + 3, size - 3)) {
        return false;
    }

    int tmp;
    int slotLength = 0;
    int i = 3 + 6;
    do {
        tmp = (input[i] & 0b00000111) << 5 | (input[i + 1] & 0b11111000) >> 3;
        slotLength += tmp;
        i++;
    } while (tmp == 255);

    uint8_t adtsHeader[7];

    int frameLength = slotLength + 7;
    int bufferFullness = 0x7FF;

    output.resize(frameLength);

    //adts header
    output.data()[0] = (0xFFF >> 4) & 0xFF;
    output.data()[1] = ((0xFFF & 0b111100000000) >> 8) << 4 | 0 << 3 /* MPEG Version */ | 0 << 1 /* Layer */ | 1 /* protection absent */;
    output.data()[2] = (convertAdtsAudioObjectType(audioObjectType) & 0b11) << 6 | sampleRate << 2 | 1 << 1 /* private bit */ | (channelConfig & 0b100) >> 2;
    output.data()[3] = (channelConfig & 0b011) << 6 | 1 << 5 /* original */ | 1 << 4 /* copy */ | 1 << 3 /* cib */ | 1 << 2 /* cis */ | (frameLength & 0b1100000000000) >> 11;
    output.data()[4] = (frameLength & 0b0011111111000) >> 3;
    output.data()[5] = (frameLength & 0b0000000000111) << 5 | (bufferFullness & 0b11111000000) >> 6;
    output.data()[6] = (bufferFullness & 0b00000111111) << 2 | 0/* rdb in frame */;

    for (int i2 = 0; i2 < slotLength; i2++) {
        output.data()[7 + i2] = (input[i + i2] & 0b00000111) << 5 | (input[i + i2 + 1] & 0b11111000) >> 3;
    }

    return true;
}

bool ADTSConverter::unpackStreamMuxConfig(uint8_t* input, int size)
{
    int audioMuxVersion = (input[0] & 0b10000000) >> 7;

    //restricted to 0
    if (audioMuxVersion) {
        return false;
    }

    int allStreamSameTimeFraming = (input[0] & 0b01000000) >> 5;
    // restricted to 0
    int numSubFrames = (input[0] & 0b00011111) << 1 | (input[1] & 0b10000000);
    // restricted to 0
    int numPrograms = input[1] & 0b01111000;
    // restricted to 0
    int numLayer = input[1] & 0b00000111;

    if (numSubFrames != 0 || numPrograms != 0 || numLayer != 0) {
        return false;
    }

    if (!unpackAudioSpecificConfig(input, size)) {
        return false;
    }

    //restricted to 0
    int frameLengthType = (input[4] & 0b11100000) >> 5;
    if (frameLengthType) {
        return false;
    }

    int a = (input[4] & 0b00011111) << 3 | (input[5] & 0b11100000) >> 5;
    int b = (input[5] & 0b00010000) >> 4;
    if (b) {
        return false;
    }

    int crcPresent = (input[5] & 0b00001000) >> 3;
    if (!crcPresent) {
        return false;
    }

    int crc = (input[5] & 0b00000111) << 3 | (input[6] & 0b11111000) >> 3;

    return true;
}

bool ADTSConverter::unpackAudioSpecificConfig(uint8_t* input, int size)
{
    audioObjectType = (input[2] & 0b11111000) >> 3;
    if (audioObjectType == 28/*AOT_ESCAPE*/) {
        return false;
    }

    sampleRate = (input[2] & 0b00000111) << 1 | (input[3] & 0b10000000) >> 7;
    if (sampleRate == 0xf) {
        return false;
    }

    channelConfig = (input[3] & 0b01111000) >> 3;

    bool framelenFlag = (input[3] & 0b00000100) >> 2;
    bool dependsOnCoder = (input[3] & 0b00000010) >> 1;
    if (framelenFlag || dependsOnCoder) {
        return false;
    }

    bool ext_flag = input[3] & 0b00000001;
    if (ext_flag) {
        return false;
    }

    return true;
}