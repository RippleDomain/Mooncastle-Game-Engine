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

	[[nodiscard]] const D3D12RenderTexture& getMainBuffer();
	[[nodiscard]] const D3D12DepthBuffer& getDepthBuffer();

	void setSize(math::u32v2 size);
	void depthPrepass(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& info);
	void render(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& info);

	void addTransitionsForDepthPrepass(d3DX::D3D12ResourceBarrier& barriers);
	void addTransitionsForDepthGPass(d3DX::D3D12ResourceBarrier& barriers);
	void addTransitionsForPostProcess(d3DX::D3D12ResourceBarrier& barriers);

	void setRenderTargetsForDepthPrepass(ID3D12GraphicsCommandList* commandList);
	void setRenderTargetsForGPass(ID3D12GraphicsCommandList* commandList);
}