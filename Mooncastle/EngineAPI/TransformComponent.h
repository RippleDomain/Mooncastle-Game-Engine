#pragma once

#include "../Components/ComponentsCommon.h"

namespace mooncastle::transform 
{
	DEFINE_TYPED_ID(transformId);

	class component final 
	{
	public:
		constexpr explicit component(transformId id) : id{ id } {}
		constexpr component() : id{ id::invalidId } {}
		constexpr transformId getId() const { return id; }
		constexpr bool isValid() const { return id::isValid(id); }

		math::v4 rotation() const;
		math::v3 orientation() const;
		math::v3 position() const;
		math::v3 scale() const;
	private:
		transformId id;
	};
}