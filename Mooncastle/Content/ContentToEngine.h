#pragma once

#include "CommonHeaders.h"

namespace mooncastle::content 
{
	struct assetType
	{
		enum type : u32
		{
			unknown = 0,
			animation,
			audio,
			material,
			mesh,
			skeleton,
			texture,
			count
		};
	};

	struct primitiveTopology 
	{
		enum type : u32 
		{
			pointList = 1,
			lineList,
			lineStrip,
			triangleList,
			triangleStrip,
			count
		};
	};

	id::idType createResource(const void* const data, assetType::type type);
	void destroyResource(id::idType id, assetType::type type);
}