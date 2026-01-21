#pragma once
#include "mmtDescriptorBase.h"
#include <list>
#include <memory>

namespace MmtTlv {

class MmtDescriptors {
public:
    MmtDescriptors() = default;

    MmtDescriptors(const MmtDescriptors&) = delete;
    MmtDescriptors& operator=(const MmtDescriptors&) = delete;

    MmtDescriptors(MmtDescriptors&&) = default;
    MmtDescriptors& operator=(MmtDescriptors&&) = default;

	bool unpack(Common::ReadStream& stream);
	std::list<std::unique_ptr<MmtDescriptorBase>> list;

};

}