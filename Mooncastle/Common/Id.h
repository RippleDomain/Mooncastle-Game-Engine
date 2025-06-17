#pragma once

#include "CommonHeaders.h"

namespace mooncastle::id
{
	using idType = u32;

	namespace detail
	{
		constexpr u32 generationBits{ 8 };
		constexpr u32 indexBits{ sizeof(idType) * 8 - generationBits };
		constexpr idType indexMask{ (idType{ 1 } << indexBits) - 1 };
		constexpr idType generationMask{ (idType{ 1 } << generationBits) - 1 };
	}

	constexpr idType invalidId{ idType(-1) };
	constexpr u32 minDeletedElements{ 1024 };

	using generationType = std::conditional_t < detail::generationBits <= 16, std::conditional_t < detail::generationBits <= 8, u8, u16 >, u32 > ;

	static_assert(detail::generationBits <= sizeof(generationType) * 8);
	static_assert(sizeof(idType) - sizeof(generationType) > 0);

	constexpr bool isValid(idType id)
	{
		return (id != invalidId);
	}
	constexpr idType index(idType id)
	{
		idType index{ id & detail::indexMask };
		assert(index != detail::indexMask);
		return id & detail::indexMask;
	}
	constexpr idType generation(idType id)
	{
		return (id >> detail::indexBits) & detail::generationMask;
	}
	constexpr idType newGeneration(idType id)
	{
		const idType currentGeneration{ id::generation(id) + 1 };
		assert(currentGeneration < (((u64)1 << detail::generationBits) - 1));
		return (index(id) | (currentGeneration << detail::indexBits));
	}

#if _DEBUG
	namespace detail
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
	struct name final : id::detail::idBase                          \
	{                                                               \
		constexpr explicit name(id::idType id) : idBase{ id } {};   \
		constexpr name() : idBase{ 0 } {};                          \
	}
#else
#define DEFINE_TYPED_ID(name) using name = id::idType;
#endif
}