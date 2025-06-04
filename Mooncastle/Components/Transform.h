#pragma once

#include "ComponentsCommon.h"

namespace mooncastle::transform
{
	struct initInfo
	{
		f32 position[3]{}; //x, y, z
		f32 rotation[4]{}; //x, y, z, w (quaternion)
		f32 scale[3]{ 1.f, 1.f, 1.f }; //x, y, z
	};

	component createTransform(const initInfo& info, gameEntity::entity entity);
	void removeTransform(component c);
}