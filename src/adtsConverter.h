#pragma once
#include <vector>
#include <cstdint>

class ADTSConverter {
public:
	bool convert(const uint8_t* input, size_t size, std::vector<uint8_t>& output);

private:
	bool unpackStreamMuxConfig(uint8_t* input, size_t size);
	bool unpackAudioSpecificConfig(uint8_t* input, size_t size);
	int audioObjectType{ 0 };
	int sampleRate{ 0 };
	int channelConfiguration{ 0 };
};