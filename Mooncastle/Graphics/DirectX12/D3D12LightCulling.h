#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12
{
	struct D3D12FrameInfo;
}

namespace mooncastle::graphics::d3D12::culling
{
	constexpr u32 lightCullingTileSize{ 16 };

	bool initialize();
	void shutdown();

	[[nodiscard]] id::idType addCuller();
	void removeCuller(id::idType id);

	void cullLights(ID3D12GraphicsCommandList* const commandList, const D3D12FrameInfo& d3D12Info, d3DX::D3D12ResourceBarrier& barriers);

	D3D12_GPU_VIRTUAL_ADDRESS getFrustums(id::idType lightCullingID, u32 frameIndex);
	/*D3D12_GPU_VIRTUAL_ADDRESS getLightGridOpaque(id::idType lightCullingID, u32 frameIndex);
	D3D12_GPU_VIRTUAL_ADDRESS getLightIndexListOpaque(id::idType lightCullingID, u32 frameIndex);*/
}