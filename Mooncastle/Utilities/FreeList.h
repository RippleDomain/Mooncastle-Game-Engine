#pragma once

#include "CommonHeaders.h"

namespace mooncastle::utl 
{
#if USE_STL_VECTOR
#pragma message("WARNING: Using utl::freeList with std::vector results in duplicate calls to the class constructor!")
#endif

	template<typename T>
	class freeList
	{
		static_assert(sizeof(T) >= sizeof(u32));

	public:
		freeList() = default;
		explicit freeList(u32 count)
		{
			array.reserve(count);
		}

		~freeList()
		{
			assert(!size);

#if USE_STL_VECTOR
			memset(array.data(), 0, array.size() * sizeof(T));
#endif
		}

		template<class... params>
		constexpr u32 add(params&&... p)
		{
			u32 id{ u32_invalid_id };

			if (nextFreeIndex == u32_invalid_id)
			{
				id = (u32)array.size();
				array.emplace_back(std::forward<params>(p)...);
			}
			else
			{
				id = nextFreeIndex;
				assert(id < array.size() && alreadyRemoved(id));
				nextFreeIndex = *(const u32* const)std::addressof(array[id]);
				new (std::addressof(array[id])) T(std::forward<params>(p)...);
			}

			++size;
			return id;
		}

		constexpr void remove(u32 id)
		{
			assert(id < array.size() && !alreadyRemoved(id));

			T& item{ array[id] };
			item.~T();
			DEBUG_OP(memset(std::addressof(array[id]), 0xcc, sizeof(T)));
			*(u32* const)std::addressof(array[id]) = nextFreeIndex;
			nextFreeIndex = id;
			--size;
		}

		constexpr u32 getSize() const
		{
			return size;
		}

		constexpr u32 getCapacity() const
		{
			return array.size();
		}

		constexpr bool isEmpty() const
		{
			return size == 0;
		}

		[[nodiscard]] constexpr T& operator[](u32 id)
		{
			assert(id < array.size() && !alreadyRemoved(id));
			return array[id];
		}

		[[nodiscard]] constexpr const T& operator[](u32 id) const
		{
			assert(id < array.size() && !alreadyRemoved(id));
			return array[id];
		}

	private:
		constexpr bool alreadyRemoved(u32 id) const
		{
			//When sizeof(T) == sizeof(u32) we cannot test if the item was already removed.
			if constexpr (sizeof(T) > sizeof(u32))
			{
				u32 i{ sizeof(u32) }; //Skips the first 4 bytes.
				const u8* const p{ (const u8* const)std::addressof(array[id]) };
				while ((p[i] == 0xcc) && (i < sizeof(T))) ++i;

				return i == sizeof(T);
			}
			else
			{
				return true;
			}
		}

#if USE_STL_VECTOR
		utl::vector<T>			array;
#else 
		utl::vector<T, false>	array;
#endif
		u32				        nextFreeIndex{ u32_invalid_id };
		u32				        size{ 0 };
	};
}