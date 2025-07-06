#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::shaders
{
	struct shaderType
	{
		enum type : u32
		{
			vertex = 0,
			hull,
			domain,
			geometry,
			pixel,
			compute,
			amplification,
			mesh,
			count
		};
	};

	struct engineShader
	{
		enum id : u32
		{
			fullscreenTriangleVS = 0,
			fillColorPS = 1,
			count
		};
	};

	bool initialize();
	void shutdown();

	D3D12_SHADER_BYTECODE getEngineShader(engineShader::id id);
}