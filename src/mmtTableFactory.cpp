#include "mmtTableFactory.h"
#include "ecm.h"
#include "mhCdt.h"
#include "mhEit.h"
#include "mhSdt.h"
#include "mhTot.h"
#include "mpt.h"
#include "plt.h"
#include "mhBit.h"
#include "mhAit.h"
#include "damt.h"
#include "ddmt.h"
#include "dcct.h"
#include "emt.h"
#include <unordered_map>
#include <functional>

namespace MmtTlv {

static const std::unordered_map<uint8_t, std::function<std::unique_ptr<MmtTableBase>()>> mapMmtTableFactory = {
	{ MmtTableId::Ecm_0,		[] { return std::make_unique<Ecm>(); } },
	{ MmtTableId::MhCdt,		[] { return std::make_unique<MhCdt>(); } },
	{ MmtTableId::MhEitPf,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_0,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_1,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_2,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_3,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_4,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_5,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_6,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_7,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_8,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_9,		[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_10,	[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_11,	[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_12,	[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_13,	[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_14,	[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhEitS_15,	[] { return std::make_unique<MhEit>(); } },
	{ MmtTableId::MhSdtActual,	[] { return std::make_unique<MhSdt>(); } },
	{ MmtTableId::MhTot,		[] { return std::make_unique<MhTot>(); } },
	{ MmtTableId::Mpt,			[] { return std::make_unique<Mpt>(); } },
	{ MmtTableId::Plt,			[] { return std::make_unique<Plt>(); } },
	{ MmtTableId::MhBit,		[] { return std::make_unique<MhBit>(); } },
	{ MmtTableId::MhAit,		[] { return std::make_unique<MhAit>(); } },
	{ MmtTableId::Damt,			[] { return std::make_unique<Damt>(); } },
	{ MmtTableId::Ddmt,			[] { return std::make_unique<Ddmt>(); } },
	{ MmtTableId::Dcct,			[] { return std::make_unique<Dcct>(); } },
	{ MmtTableId::Emt,			[] { return std::make_unique<Emt>(); } },
};

std::unique_ptr<MmtTableBase> MmtTableFactory::create(uint8_t id) {
	auto it = mapMmtTableFactory.find(id);
	if (it == mapMmtTableFactory.end()) {
		return {};
	}

	return it->second();
}

bool MmtTableFactory::isValidId(uint8_t id) {
	auto it = mapMmtTableFactory.find(id);
	if (it == mapMmtTableFactory.end()) {
		return false;
	}

	return true;
}

}