#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12
{
	struct D3D12FrameInfo;
}

namespace mooncastle::graphics::d3D12::light 
{
	bool initialize();
	void shutdown();

	graphics::light create(lightInitInfo info);
	void remove(lightId id, u64 lightSetKey);
	void setParameter(lightId id, u64 lightSetKey, lightParameter::parameter parameter, const void* const data, u32 dataSize);
	void getParameter(lightId id, u64 lightSetKey, lightParameter::parameter parameter, void* const data, u32 dataSize);

	void updateLightBuffers(const D3D12FrameInfo& d3D12Info);
	D3D12_GPU_VIRTUAL_ADDRESS getNonCullableLightBuffer(u32 frameIndex);
	u32 getNonCullableLightCount(u64 lightSetKey);
}