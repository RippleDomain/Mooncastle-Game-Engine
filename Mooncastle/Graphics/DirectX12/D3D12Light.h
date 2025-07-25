#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::light 
{
	graphics::light create(lightInitInfo info);
	void remove(lightId id, u64 lightSetKey);
	void setParameter(lightId id, u64 lightSetKey, lightParameter::parameter parameter, const void* const data, u32 dataSize);
	void getParameter(lightId id, u64 lightSetKey, lightParameter::parameter parameter, void* const data, u32 dataSize);
}