#pragma once

#include "ComponentsCommon.h"

namespace mooncastle
{
#define INIT_INFO(component) namespace component { struct initInfo; }
	INIT_INFO(transform)
	INIT_INFO(script)
#undef INIT_INFO

	namespace transform { struct initInfo; }

	namespace gameEntity 
	{
		struct entityInfo
		{
			transform::initInfo* transform{ nullptr };
			script::initInfo* script{ nullptr };
		};

		entity create(entityInfo info);
		void remove(entityId id);
		bool isAlive(entityId id);
	}
}