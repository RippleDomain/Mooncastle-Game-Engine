#pragma once

#include "ComponentsCommon.h"

namespace mooncastle::script
{
	struct initInfo
	{
		detail::script_creator scriptCreator;
	};

	component create(initInfo info, gameEntity::entity entity);
	void remove(component c);
}