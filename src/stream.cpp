#include "stream.h"

uint16_t swapEndian16(uint16_t num) {
    return (num >> 8) | (num << 8);
}

uint32_t swapEndian32(uint32_t num) {
    return ((num >> 24) & 0x000000FF) |
        ((num >> 8) & 0x0000FF00) |
        ((num << 8) & 0x00FF0000) |
        ((num << 24) & 0xFF000000);
}

uint64_t swapEndian64(uint64_t num) {
    return ((num >> 56) & 0x00000000000000FFULL) |
        ((num >> 40) & 0x000000000000FF00ULL) |
        ((num >> 24) & 0x0000000000FF0000ULL) |
        ((num >> 8) & 0x00000000FF000000ULL) |
        ((num << 8) & 0x000000FF00000000ULL) |
        ((num << 24) & 0x0000FF0000000000ULL) |
        ((num << 40) & 0x00FF000000000000ULL) |
        ((num << 56) & 0xFF00000000000000ULL);
}

Stream::Stream(const std::vector<uint8_t>& data)
{
	this->data = data;
	this->hasSize = true;
	this->size = data.size();
}

Stream::Stream(const std::vector<uint8_t>& data, uint32_t size)
{
	if (data.size() < size) {
		throw std::runtime_error("wrong size");
	}

    this->data = data;
	this->hasSize = true;
	this->size = size;
}

Stream::Stream(Stream& stream, uint32_t size)
{
    if (stream.data.size() < stream.cur + size) {
        throw std::runtime_error("wrong size");
    }
    this->data.insert(this->data.begin(), stream.data.begin() + stream.cur, stream.data.begin() + stream.cur + size);
    this->hasSize = true;
    this->size = size;
}

Stream::Stream(Stream& stream)
{
    this->data = stream.data;
    this->hasSize = stream.hasSize;
    this->size = stream.size;
    this->cur = stream.cur;
}
