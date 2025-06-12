#pragma once

#include "..\Components\ComponentsCommon.h"

namespace mooncastle::script
{
	DEFINE_TYPED_ID(scriptId);

	class component final
	{
	public:
		constexpr explicit component(scriptId id) : _id{ id } {}
		constexpr component() : _id{ id::invalidId } {}
		constexpr scriptId getId() const { return _id; }
		constexpr bool isValid() const { return id::isValid(_id); }
	private:
		scriptId _id;
	};
}