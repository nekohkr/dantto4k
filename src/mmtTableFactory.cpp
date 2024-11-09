#include "mmtTableFactory.h"
#include "ecm.h"
#include "mhCdt.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "mhTot.h"
#include "mpt.h"
#include "plt.h"
#include "mhBit.h"
#include <unordered_map>
#include <functional>

namespace MmtTlv {

static const std::unordered_map<uint8_t, std::function<std::shared_ptr<MmtTableBase>()>> mapMmtTableFactory = {
	{ MmtTableId::Ecm,		[] { return std::make_shared<Ecm>(); } },
	{ MmtTableId::MhCdt,	[] { return std::make_shared<MhCdt>(); } },
	{ MmtTableId::MhEit,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_1,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_2,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_3,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_4,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_5,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_6,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_7,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_8,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_9,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_10,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_11,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_12,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_13,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_14,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_15,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhEit_16,	[] { return std::make_shared<MhEit>(); } },
	{ MmtTableId::MhSdt,	[] { return std::make_shared<MhSdt>(); } },
	{ MmtTableId::MhTot,	[] { return std::make_shared<MhTot>(); } },
	{ MmtTableId::Mpt,		[] { return std::make_shared<Mpt>(); } },
	{ MmtTableId::Plt,		[] { return std::make_shared<Plt>(); } },
	{ MmtTableId::MhBit,	[] { return std::make_shared<MhBit>(); } },
};

std::shared_ptr<MmtTableBase> MmtTableFactory::create(uint8_t id) {
	auto it = mapMmtTableFactory.find(id);
	if (it == mapMmtTableFactory.end()) {
		return {};
	}

	return it->second();
}

bool MmtTableFactory::isValidId(uint8_t id)
{
	auto it = mapMmtTableFactory.find(id);
	if (it == mapMmtTableFactory.end()) {
		return false;
	}

	return true;
}

}