#pragma once
#include "mmtDescriptor.h"

class MhShortEventDescriptor : public MmtDescriptor {
public:
	~MhShortEventDescriptor();
	bool unpack(Stream& stream);

	char language[4];
	std::string eventName;
	std::string text;

};