#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::shaders
{
	struct engineShader
	{
		enum id : u32
		{
			fullscreenTriangleVS = 0,
			fillColorPS = 1,
			postProcessPS = 2,
			gridFrustumsCS = 3,
			count
		};
	};

	bool initialize();
	void shutdown();

	D3D12_SHADER_BYTECODE getEngineShader(engineShader::id id);
}