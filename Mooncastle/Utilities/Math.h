#pragma once

#include "CommonHeaders.h"
#include "MathTypes.h"

namespace mooncastle::math 
{
    template<typename T>
    constexpr T clamp(T value, T min, T max)
    {
        return (value < min) ? min : (value > max) ? max : value;
    }

	template<u32 bits>
	constexpr u32 packUnitFloat(f32 f)
	{
		static_assert(bits <= sizeof(u32) * 8);
		assert(f >= 0.f && f <= 1.f);
		constexpr f32 intervals{ (f32)((1ui32 << bits) - 1) };

		return (u32)(intervals * f + 0.5f);
	}

	template<u32 bits>
	constexpr f32 unpackToUnitFloat(u32 i)
	{
		static_assert(bits <= sizeof(u32) * 8);
		assert(i < (1ui32 << bits));
		constexpr f32 intervals{ (f32)((1ui32 << bits) - 1) };

		return (f32)i / intervals;
	}

	template<u32 bits>
	constexpr u32 packFloat(f32 f, f32 min, f32 max)
	{
		assert(min < max);
		assert(f <= max && f >= min);
		const f32 distance{ (f - min) / (max - min) };

		return packUnitFloat<bits>(distance);
	}

	template<u32 bits>
	constexpr f32 unpackToFloat(u32 i, f32 min, f32 max)
	{
		assert(min < max);
		return unpackToUnitFloat<bits>(i) * (max - min) + min;
	}
}