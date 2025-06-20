#pragma once

#include "ToolsCommon.h"

namespace mooncastle::tools 
{
	enum primitiveMeshType : u32
	{
		plane,
		cube,
		uvSphere,
		icoSphere,
		cylinder,
		capsule,
		count
	};

	struct primitiveInitInfo
	{
		primitiveMeshType      type;
		u32                    segments[3]{ 1, 1, 1 };
		math::v3               size{ 1, 1, 1 };
		u32                    lod{ 0 };
	};
}