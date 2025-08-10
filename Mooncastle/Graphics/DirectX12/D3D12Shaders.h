#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::shaders
{
	struct engineShader
	{
		enum id : u32
		{
			fullscreenTriangleVS = 0,
			postProcessPS,
			gridFrustumsCS,
			lightCullingCS,
			count
		};
	};

	bool initialize();
	void shutdown();

	D3D12_SHADER_BYTECODE getEngineShader(engineShader::id id);
}