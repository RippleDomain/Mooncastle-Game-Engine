#pragma once

#include "CommonHeaders.h"

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

struct shaderFileInfo
{
    const char* fileName;
    const char* function;
    shaderType::type type;
};

std::unique_ptr<u8[]> compileShader(shaderFileInfo info, const char* filePath, mooncastle::utl::vector<std::wstring>& extraArgs);
bool compileShaders();