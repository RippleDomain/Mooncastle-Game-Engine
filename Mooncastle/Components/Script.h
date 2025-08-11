#pragma once

#include "ComponentsCommon.h"

namespace mooncastle::script
{
	struct initInfo
	{
		detail::scriptCreator scriptCreator;
	};

	component create(initInfo info, gameEntity::entity entity);
	void remove(component c);
	void update(f32 dt);
}