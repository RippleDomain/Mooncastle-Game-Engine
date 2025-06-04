 #pragma once

#include "..\Components\ComponentsCommon.h"
#include "TransformComponent.h"

namespace mooncastle::gameEntity 
{
	DEFINE_TYPED_ID(entityId);

	class entity 
	{
	public:
		constexpr explicit entity(entityId id) : _id{ id } {}
		constexpr entity() : _id{ id::invalidId } {}
		constexpr entityId getId() const { return _id; }
		constexpr bool isValid() const { return id::isValid(_id); }

		transform::component transform() const;
	private:
		entityId _id;
	};
}
