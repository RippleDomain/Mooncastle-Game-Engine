#pragma once

#include "CommonHeaders.h"

namespace mooncastle::content 
{
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

}