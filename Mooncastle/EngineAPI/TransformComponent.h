#pragma once

#include "..\Components\ComponentsCommon.h"

namespace mooncastle::transform 
{
	DEFINE_TYPED_ID(transformId);

	class component final 
	{
	public:
		constexpr explicit component(transformId id) : _id{ id } {}
		constexpr component() : _id{ id::invalidId } {}
		constexpr transformId getId() const { return _id; }
		constexpr bool isValid() const { return id::isValid(_id); }

		math::v4 rotation() const;
		math::v3 position() const;
		math::v3 scale() const;
	private:
		transformId _id;
	};
}