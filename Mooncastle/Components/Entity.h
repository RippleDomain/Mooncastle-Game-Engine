#pragma once

#include "ComponentsCommon.h"

namespace mooncastle
{
#define INIT_INFO(component) namespace component { struct initInfo; }
	INIT_INFO(transform)
#undef INIT_INFO

	namespace transform { struct initInfo; }

	namespace gameEntity 
	{
		struct entityInfo
		{
			transform::initInfo* transform{ nullptr };
		};

		EntityId createGameEntity(const entityInfo& info);
		void removeGameEntity(EntityId entityId);
		bool isAlive(EntityId entityId);
	}
}