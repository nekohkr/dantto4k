#include "stream.h"

namespace MmtTlv {
    
namespace Common {

Stream::Stream(const std::vector<uint8_t>& data)
    : data(data)
{
    this->hasSize = true;
    this->size = data.size();
}

Stream::Stream(const std::vector<uint8_t>& data, uint32_t size)
    : data(data)
{
    if (data.size() < size) {
        throw std::out_of_range("wrong size");
    }

    this->hasSize = true;
    this->size = size;
}

Stream::Stream(Stream& stream, uint32_t size)
    : data(stream.data)
{
    if (stream.data.size() < stream.cur + size) {
        throw std::out_of_range("wrong size");
    }

    this->cur = stream.cur;
    this->hasSize = true;
    this->size = stream.cur + size;
}

Stream::Stream(Stream& stream)
    : data(stream.data)
{
    this->hasSize = stream.hasSize;
    this->size = stream.size;
    this->cur = stream.cur;
}

}

}