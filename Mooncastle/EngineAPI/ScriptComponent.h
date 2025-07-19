#pragma once

#include "../Components/ComponentsCommon.h"

namespace mooncastle::script
{
	DEFINE_TYPED_ID(scriptId);

	class component final
	{
	public:
		constexpr explicit component(scriptId id) : id{ id } {}
		constexpr component() : id{ id::invalidId } {}
		constexpr scriptId getId() const { return id; }
		constexpr bool isValid() const { return id::isValid(id); }
	private:
		scriptId id;
	};
}