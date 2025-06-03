#pragma once

#include "CommonHeaders.h"

namespace mooncastle::id
{
	using idType = u32;
	constexpr u32 generationBits{ 8 };
	constexpr u32 indexBits{ sizeof(idType) * 8 - generationBits };
	constexpr idType indexMask{ idType{ 1 } << indexBits - 1 };
	constexpr idType generationMask{ idType{ 1 } << generationBits - 1 };
	constexpr idType idMask{ idType{ -1 } };

	using generationType = std::conditional_t < generationBits <= 16, std::conditional_t < generationBits <= 8, u8, u16 >, u32 > ;

	static_assert(generationBits <= sizeof(generationType) * 8);
	static_assert(sizeof(idType) - sizeof(generationType) > 0);

	inline bool isValid(idType id)
	{
		return (id != idMask);
	}
	inline idType index(idType id)
	{
		return id & indexMask;
	}
	inline idType generation(idType id)
	{
		return (id >> indexBits) & generationMask;
	}
	inline idType newGeneration(idType id)
	{
		const idType currentGeneration{ id::generation(id) + 1 };
		assert(currentGeneration < 255);

		return (index(id) | (currentGeneration << indexBits));
	}

#if _DEBUG
	namespace internal 
	{
		struct idBase
		{
			constexpr explicit idBase(idType id) : _id{ id } {};
			constexpr operator idType() const { return _id; }

		private:
			idType _id;
		};
	}

#define DEFINE_TYPED_ID(name)                                       \
	struct name final : id::internal::idBase                        \
	{                                                               \
		constexpr explicit name(id::idType id) : idBase{ id } {};   \
		constexpr name() : idBase{ id::idMask } {};                 \
	}
#else
#define DEFINE_TYPED_ID(name) using name = id::idType;
#endif
}