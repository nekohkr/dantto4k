#pragma once
#include <vector>

class ADTSConverter {
public:
	bool convert(uint8_t* input, int size, std::vector<uint8_t>& output);

protected:
	bool unpackStreamMuxConfig(uint8_t* input, int size);
	bool unpackAudioSpecificConfig(uint8_t* input, int size);

	int audioObjectType = 0;
	int sampleRate = 0;
	int channelConfig = 0;
};