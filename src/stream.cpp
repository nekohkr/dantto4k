#include "stream.h"

namespace MmtTlv {
    
namespace Common {

ReadStream::ReadStream(const std::vector<uint8_t>& buffer)
    : buffer(buffer)
{
    this->hasSize = true;
    this->size = buffer.size();
}

ReadStream::ReadStream(const std::vector<uint8_t>& buffer, uint32_t size)
    : buffer(buffer)
{
    if (buffer.size() < size) {
        throw std::out_of_range("Access out of bounds");
    }

    this->hasSize = true;
    this->size = size;
}

ReadStream::ReadStream(ReadStream& stream, uint32_t size)
    : buffer(stream.buffer)
{
    if (stream.buffer.size() < stream.pos + size) {
        throw std::out_of_range("Access out of bounds");
    }

    this->pos = stream.pos;
    this->hasSize = true;
    this->size = stream.pos + size;
}

ReadStream::ReadStream(ReadStream& stream)
    : buffer(stream.buffer)
{
    this->hasSize = stream.hasSize;
    this->size = stream.size;
    this->pos = stream.pos;
}

}

}