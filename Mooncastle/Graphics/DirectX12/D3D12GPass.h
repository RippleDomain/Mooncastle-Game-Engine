#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12
{
	struct D3D12FrameInfo;
}

namespace mooncastle::graphics::d3D12::gPass
{
	bool initialize();
	void shutdown();
	void setSize(math::u32v2 size);
	void depthPrepass(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& info);
	void render(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& info);
}