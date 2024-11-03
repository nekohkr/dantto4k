#pragma once

namespace MmtTlv {

enum class FragmentType
{
	MpuMetadata = 0x00,
	MovieFragmentMetadata = 0x01,
	Mfu = 0x02
};

enum FragmentationIndicator {
	NotFragmented = 0b00,
	FirstFragment = 0b01,
	MiddleFragment = 0b10,
	LastFragment = 0b11,
};

}