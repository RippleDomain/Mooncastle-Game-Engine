#include <filesystem>
#include <fstream>

#include "..\packages\DirectXShaderCompiler\inc\d3d12shader.h"
#include "..\packages\DirectXShaderCompiler\inc\dxcapi.h"
#include "Graphics\DirectX12\D3D12Core.h"
#include "Graphics\DirectX12\D3D12Shaders.h"

#pragma comment(lib, "../packages/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

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
        {"FullScreenTriangle.hlsl", "FullScreenTriangleVS", engineShader::fullscreenTriangleVS, shaderType::vertex},
        {"FillColor.hlsl", "FillColorPS", engineShader::fillColorPS, shaderType::pixel},
        {"PostProcess.hlsl", "PostProcessPS", engineShader::postProcessPS, shaderType::pixel}
    };

    static_assert(_countof(shaderFiles) == engineShader::count);

    constexpr const char* shadersSourcePath{ "../../Mooncastle/Graphics/DirectX12/Shaders/" };

	std::wstring toWString(const char* c)
	{
		std::string s{ c };
		return { s.begin(), s.end() };
	}

	class shaderCompiler
	{
	public:
		shaderCompiler()
		{
			HRESULT hr{ S_OK };

			DXCall(hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
			if (FAILED(hr)) return;

			DXCall(hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
			if (FAILED(hr)) return;

			DXCall(hr = utils->CreateDefaultIncludeHandler(&includeHandler));
			if (FAILED(hr)) return;
		}

		DISABLE_COPY_AND_MOVE(shaderCompiler);

		IDxcBlob* compile(shaderFileInfo info, std::filesystem::path fullPath)
		{
			assert(compiler && utils && includeHandler);
			HRESULT hr{ S_OK };

			// Load the source file using Utils interface.
			ComPtr<IDxcBlobEncoding> sourceBlob{ nullptr };
			DXCall(hr = utils->LoadFile(fullPath.c_str(), nullptr, &sourceBlob));
			if (FAILED(hr)) return nullptr;

			assert(sourceBlob && sourceBlob->GetBufferSize());

			std::wstring file{ toWString(info.file) };
			std::wstring func{ toWString(info.function) };
			std::wstring prof{ toWString(profileStrings[(u32)info.type]) };
			std::wstring inc{ toWString(shadersSourcePath) };

			LPCWSTR args[]
			{
				file.c_str(),                //Optional shader source file name for error reporting.
				L"-E", func.c_str(),         //Entry function.
				L"-T", prof.c_str(),         //Target profile.
				L"-I", inc.c_str(),          //Include path.
				DXC_ARG_ALL_RESOURCES_BOUND,
#if _DEBUG
				DXC_ARG_DEBUG,
				DXC_ARG_SKIP_OPTIMIZATIONS,
#else
				DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
				DXC_ARG_WARNINGS_ARE_ERRORS,
				L"-Qstrip_reflect",          //Strip reflections into a separate blob.
				L"-Qstrip_debug",            //Strip debug information into a separate blob.
			};

			OutputDebugStringA("Compiling ");
			OutputDebugStringA(info.file);

			return compile(sourceBlob.Get(), args, _countof(args));
		}

		IDxcBlob* compile(IDxcBlobEncoding* sourceBlob, LPCWSTR* args, u32 numArgs)
		{
			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = sourceBlob->GetBufferPointer();
			buffer.Size = sourceBlob->GetBufferSize();

			HRESULT hr{ S_OK };
			ComPtr<IDxcResult> results{ nullptr };
			DXCall(hr = compiler->Compile(&buffer, args, numArgs, includeHandler.Get(), IID_PPV_ARGS(&results)));
			if (FAILED(hr)) return nullptr;

			ComPtr<IDxcBlobUtf8> errors{ nullptr };
			DXCall(hr = results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
			if (FAILED(hr)) return nullptr;

			if (errors && errors->GetStringLength())
			{
				OutputDebugStringA("\nShader compilation error: \n");
				OutputDebugStringA(errors->GetStringPointer());
			}
			else
			{
				OutputDebugStringA(" [ Succeeded ]");
			}
			OutputDebugStringA("\n");

			HRESULT status{ S_OK };
			DXCall(hr = results->GetStatus(&status));
			if (FAILED(hr) || FAILED(status)) return nullptr;

			ComPtr<IDxcBlob> shader{ nullptr };
			DXCall(hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
			if (FAILED(hr)) return nullptr;

			return shader.Detach();
		}

	private:
		constexpr static const char* profileStrings[]{ "vs_6_5", "hs_6_5", "ds_6_5", "gs_6_5", "ps_6_5", "cs_6_5", "as_6_5", "ms_6_5" };
		static_assert(_countof(profileStrings) == shaderType::count);

		ComPtr<IDxcCompiler3>       compiler{ nullptr };
		ComPtr<IDxcUtils>           utils{ nullptr };
		ComPtr<IDxcIncludeHandler>  includeHandler{ nullptr };
	};

	decltype(auto) getEngineShadersPath()
	{
		return std::filesystem::path{ graphics::getEngineShadersPath(graphics::graphicsPlatform::direct3D12) };
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

			fullPath = path;
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

	shaderCompiler compiler{};

    //Compile shaders and them together in a buffer in the same order of compilation.
    for (u32 i{ 0 }; i < engineShader::count; ++i)
    {
        auto& info = shaderFiles[i];

        path = shadersSourcePath;
        path += info.file;
        fullPath = path;
        if (!std::filesystem::exists(fullPath)) return false;

        ComPtr<IDxcBlob> compiledShader{ compiler.compile(info, fullPath) };

        if (compiledShader && compiledShader->GetBufferPointer() && compiledShader->GetBufferSize())
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
