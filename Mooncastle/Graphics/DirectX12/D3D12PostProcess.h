#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12
{
	struct D3D12FrameInfo;
}

namespace mooncastle::graphics::d3D12::ppfx
{
	bool initialize();
	void shutdown();
	void postProcess(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& d3D12Info, D3D12_CPU_DESCRIPTOR_HANDLE targetRTV);
}