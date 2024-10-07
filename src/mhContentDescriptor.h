#pragma once
#include "mmtDescriptor.h"
#include <list>

class MhContentItem {
public:
    bool unpack(Stream& stream);

    uint8_t content_nibble_level_1;
    uint8_t content_nibble_level_2;
    uint8_t user_nibble1;
    uint8_t user_nibble2;
};

class MhContentDescriptor : public MmtDescriptor {
public:
    bool unpack(Stream& stream);

    std::list<MhContentItem> items;

};