#pragma once

#include <cstdint>
#include <utility>

#define NOMINMAX
#undef min
#undef max

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

struct uint128
{
	uint64 low = 0ull;
	uint64 high = 0ull;

	bool operator==(const uint128&) const = default;
};

template<>
struct std::hash<uint128>
{
	uint64 operator()(const uint128& v) const noexcept
	{
		uint64 seed = std::hash<uint64>{}(v.low);
		seed ^= std::hash<uint64>{}(v.high) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
		return seed;
	}
};

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

// TODO: use <stdfloat> once it is implemented?
using float32 = float;
using float64 = double;
