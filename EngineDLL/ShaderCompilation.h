#pragma once

#include "CommonHeaders.h"
#include "Graphics/Renderer.h"

struct shaderFileInfo
{
	const char* fileName;
	const char* function;
	mooncastle::graphics::shaderType::type type;
};

std::unique_ptr<u8[]> compileShader(shaderFileInfo info, u8* code, u32 codeSize, mooncastle::utl::vector<std::wstring>& extraArgs,
	bool includeErrorsAndDisassembly = false);
std::unique_ptr<u8[]> compileShader(shaderFileInfo info, const char* filePath, mooncastle::utl::vector<std::wstring>& extraArgs,
	bool includeErrorsAndDisassembly = false);
bool compileShaders();