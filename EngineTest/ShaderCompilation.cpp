#include <d3d12shader.h>
#include <dxcapi.h>
#include <filesystem>
#include <fstream>

#include "Graphics\DirectX12\D3D12Core.h"
#include "Graphics\DirectX12\D3D12Shaders.h"

using namespace mooncastle;
using namespace mooncastle::graphics::d3D12::shaders;
using namespace Microsoft::WRL;

namespace
{
    struct shaderFileInfo
    {
        const char* file;
        const char* function;
        engineShader::id id;
        shaderType::type type;
    };

    constexpr shaderFileInfo shaderFiles[]
    {
        {"FullScreenTriangle.hlsl", "fullscreenTriangleVS", engineShader::fullscreenTriangleVS, shaderType::vertex}
    };

    static_assert(_countof(shaderFiles) == engineShader::count);

    constexpr const char* shadersSourcePath{ "../../Engine/Graphics/DirectX12/Shaders/" };

	decltype(auto) getEngineShadersPath()
	{
		return std::filesystem::absolute(graphics::getEngineShadersPath(graphics::graphicsPlatform::direct3D12));
	}

	bool compiledShadersAreUpToDate()
	{
		auto engineShadersPath = getEngineShadersPath();
		if (!std::filesystem::exists(engineShadersPath)) return false;

		auto shadersCompilationTime = std::filesystem::last_write_time(engineShadersPath);

		std::filesystem::path path{};
		std::filesystem::path fullPath{};

		for (u32 i{ 0 }; i < engineShader::count; ++i)
		{
			auto& info = shaderFiles[i];
			path = shadersSourcePath;
			path += info.file;

			fullPath = std::filesystem::absolute(path);
			if (!std::filesystem::exists(fullPath)) return false;

			auto shaderFileTime = std::filesystem::last_write_time(fullPath);

			if (shaderFileTime > shadersCompilationTime)
			{
				return false;
			}
		}

		return true;
	}

	bool saveCompiledShaders(utl::vector<ComPtr<IDxcBlob>>& shaders)
	{
		auto engineShadersPath = getEngineShadersPath();

		std::filesystem::create_directories(engineShadersPath.parent_path());
		std::ofstream file(engineShadersPath, std::ios::out | std::ios::binary);

		if (!file || !std::filesystem::exists(engineShadersPath))
		{
			file.close();
			return false;
		}

		for (auto& shader : shaders)
		{
			const D3D12_SHADER_BYTECODE byte_code{ shader->GetBufferPointer(), shader->GetBufferSize() };

			file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
			file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
		}
		file.close();

		return true;
	}
}

bool compileShaders()
{
    if (compiledShadersAreUpToDate()) return true;

    utl::vector<ComPtr<IDxcBlob>> shaders;
    std::filesystem::path path{};
    std::filesystem::path fullPath{};

    //Compile shaders and them together in a buffer in the same order of compilation.
    for (u32 i{ 0 }; i < engineShader::count; ++i)
    {
        auto& info = shaderFiles[i];

        path = shadersSourcePath;
        path += info.file;
        fullPath = std::filesystem::absolute(path);
        if (!std::filesystem::exists(fullPath)) return false;

        ComPtr<IDxcBlob> compiledShader{ /*Call compile shader function*/ };

        if (compiledShader != nullptr && compiledShader->GetBufferPointer() && compiledShader->GetBufferSize())
        {
            shaders.emplace_back(std::move(compiledShader));
        }
        else
        {
            return false;
        }
    }

    return saveCompiledShaders(shaders);
}
