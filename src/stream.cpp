#include "stream.h"

namespace MmtTlv {
    
namespace Common {

ReadStream::ReadStream(std::span<const uint8_t> buffer)
    : buffer(buffer), hasSize(true), size(buffer.size())
{
}

ReadStream::ReadStream(const std::vector<uint8_t>& buffer)
    : ReadStream(std::span<const uint8_t>(buffer)) {}

ReadStream::ReadStream(const std::vector<uint8_t>& buffer, size_t size)
    : buffer(buffer)
{
    if (this->buffer.size() < size) {
        throw std::out_of_range("Access out of bounds");
    }

    this->hasSize = true;
    this->size = size;
}

ReadStream::ReadStream(ReadStream& stream, size_t size)
    : buffer(stream.buffer)
{
    if (stream.size < stream.pos + size) {
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
