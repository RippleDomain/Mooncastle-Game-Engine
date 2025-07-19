#include <filesystem>
#include <fstream>

#include "..\packages\DirectXShaderCompiler\inc\d3d12shader.h"
#include "..\packages\DirectXShaderCompiler\inc\dxcapi.h"

#include "Graphics\DirectX12\D3D12Core.h"
#include "Graphics\DirectX12\D3D12Shaders.h"
#include "Content\ContentToEngine.h"
#include "Utilities\IOStream.h"
#include "ShaderCompilation.h"

#pragma comment(lib, "../packages/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

using namespace mooncastle;
using namespace mooncastle::graphics::d3D12::shaders;
using namespace Microsoft::WRL;

namespace
{
	constexpr const char* shadersSourcePath{ "../../Mooncastle/Graphics/DirectX12/Shaders/" };

	struct engineShaderInfo
	{
		engineShader::id id;
		shaderFileInfo info;
	};

	constexpr engineShaderInfo engineShaderFiles[]
    {
        engineShader::fullscreenTriangleVS, {"FullScreenTriangle.hlsl", "FullScreenTriangleVS", shaderType::vertex},
		engineShader::fillColorPS, {"FillColor.hlsl", "FillColorPS", shaderType::pixel},
		engineShader::postProcessPS, {"PostProcess.hlsl", "PostProcessPS", shaderType::pixel}
    };

    static_assert(_countof(engineShaderFiles) == engineShader::count);

	struct dxcCompiledShader
	{
		ComPtr<IDxcBlob> byteCode;
		ComPtr<IDxcBlobUtf8> disassembly;
		DxcShaderHash hash;
	};

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

		dxcCompiledShader compile(shaderFileInfo info, std::filesystem::path fullPath)
		{
			assert(compiler && utils && includeHandler);
			HRESULT hr{ S_OK };

			// Load the source file using Utils interface.
			ComPtr<IDxcBlobEncoding> sourceBlob{ nullptr };
			DXCall(hr = utils->LoadFile(fullPath.c_str(), nullptr, &sourceBlob));
			if (FAILED(hr)) return {};

			assert(sourceBlob && sourceBlob->GetBufferSize());

			std::wstring file{ toWString(info.fileName) };
			std::wstring func{ toWString(info.function) };
			std::wstring prof{ toWString(profileStrings[(u32)info.type]) };
			std::wstring inc{ toWString(shadersSourcePath) };

			LPCWSTR args[]
			{
				file.c_str(),                //Optional shader source file name for error reporting.
				L"-E", func.c_str(),         //Entry function.
				L"-T", prof.c_str(),         //Target profile.
				L"-I", inc.c_str(),          //Include path.
				L"-enable-16bit-types",      //Enable 16-bit type support.
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
			OutputDebugStringA(info.fileName);
			OutputDebugStringA(" : ");
			OutputDebugStringA(info.function);
			OutputDebugStringA("\n");

			return compile(sourceBlob.Get(), args, _countof(args));
		}

		dxcCompiledShader compile(IDxcBlobEncoding* sourceBlob, LPCWSTR* args, u32 numArgs)
		{
			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = sourceBlob->GetBufferPointer();
			buffer.Size = sourceBlob->GetBufferSize();

			HRESULT hr{ S_OK };
			ComPtr<IDxcResult> results{ nullptr };
			DXCall(hr = compiler->Compile(&buffer, args, numArgs, includeHandler.Get(), IID_PPV_ARGS(&results)));
			if (FAILED(hr)) return {};

			ComPtr<IDxcBlobUtf8> errors{ nullptr };
			DXCall(hr = results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
			if (FAILED(hr)) return {};

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
			if (FAILED(hr) || FAILED(status)) return {};

			ComPtr<IDxcBlob> hash{ nullptr };
			DXCall(hr = results->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&hash), nullptr));
			if (FAILED(hr)) return {};

			DxcShaderHash *const hashBuffer{ (DxcShaderHash *const)hash->GetBufferPointer() };

			//Different source code could result in the same byte code, so we only care about byte code hash.
			assert(!(hashBuffer->Flags & DXC_HASHFLAG_INCLUDES_SOURCE));

			OutputDebugStringA("Shader hash: ");
			for (u32 i{ 0 }; i < _countof(hashBuffer->HashDigest); ++i)
			{
				char hashBytes[3]{}; //2 chars for hex value plus termination 0.
				sprintf_s(hashBytes, "%02x", (u32)hashBuffer->HashDigest[i]);
				OutputDebugStringA(hashBytes);
				OutputDebugStringA(" ");
			}
			OutputDebugStringA("\n");

			ComPtr<IDxcBlob> shader{ nullptr };

			DXCall(hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
			if (FAILED(hr)) return {};

			buffer.Ptr = shader->GetBufferPointer();
			buffer.Size = shader->GetBufferSize();

			ComPtr<IDxcResult> disassemblyResults{ nullptr };
			DXCall(hr = compiler->Disassemble(&buffer, IID_PPV_ARGS(&disassemblyResults)));

			ComPtr<IDxcBlobUtf8> disassembly{ nullptr };
			DXCall(hr = disassemblyResults->GetOutput(DXC_OUT_DISASSEMBLY, IID_PPV_ARGS(&disassembly), nullptr));

			dxcCompiledShader result{ shader.Detach(), disassembly.Detach() };
			memcpy(&result.hash.HashDigest[0], &hashBuffer->HashDigest[0], _countof(hashBuffer->HashDigest));

			return result;
		}

	private:
		constexpr static const char* profileStrings[]{ "vs_6_6", "hs_6_6", "ds_6_6", "gs_6_6", "ps_6_6", "cs_6_6", "as_6_6", "ms_6_6" };
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

		std::filesystem::path fullPath{};

		for (u32 i{ 0 }; i < engineShader::count; ++i)
		{
			auto& file = engineShaderFiles[i];

			fullPath = shadersSourcePath;
			fullPath += file.info.fileName;

			if (!std::filesystem::exists(fullPath)) return false;

			auto shaderFileTime = std::filesystem::last_write_time(fullPath);

			if (shaderFileTime > shadersCompilationTime)
			{
				return false;
			}
		}

		return true;
	}

	bool saveCompiledShaders(utl::vector<dxcCompiledShader>& shaders)
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
			const D3D12_SHADER_BYTECODE byteCode{ shader.byteCode->GetBufferPointer(), shader.byteCode->GetBufferSize()};

			file.write((char*)&byteCode.BytecodeLength, sizeof(byteCode.BytecodeLength));
			file.write((char*)&shader.hash.HashDigest[0], _countof(shader.hash.HashDigest));
			file.write((char*)byteCode.pShaderBytecode, byteCode.BytecodeLength);
		}
		file.close();

		return true;
	}
}

std::unique_ptr<u8[]> compileShader(shaderFileInfo info, const char* filePath)
{
	std::filesystem::path fullPath{ filePath };
	fullPath += info.fileName;

	if (!std::filesystem::exists(fullPath)) return {};

	shaderCompiler compiler{};
	dxcCompiledShader compiledShader{ compiler.compile(info, fullPath) };

	if (compiledShader.byteCode && compiledShader.byteCode->GetBufferPointer() && compiledShader.byteCode->GetBufferSize())
	{
		static_assert(content::compiledShader::hashLength == _countof(DxcShaderHash::HashDigest));

		const u64 bufferSize{ sizeof(u64) + content::compiledShader::hashLength + compiledShader.byteCode->GetBufferSize() };
		std::unique_ptr<u8[]> buffer{ std::make_unique<u8[]>(bufferSize) };
		utl::blobStreamWriter blob{ buffer.get(), bufferSize };

		blob.write(compiledShader.byteCode->GetBufferSize());
		blob.write(compiledShader.hash.HashDigest, content::compiledShader::hashLength);
		blob.write((u8*)compiledShader.byteCode->GetBufferPointer(), compiledShader.byteCode->GetBufferSize());

		assert(blob.getOffset() == bufferSize);

		return buffer;
	}

	return {};
}

bool compileShaders()
{
    if (compiledShadersAreUpToDate()) return true;

	shaderCompiler compiler{};
    utl::vector<dxcCompiledShader> shaders;
    std::filesystem::path fullPath{};

    //Compile shaders and them together in a buffer in the same order of compilation.
    for (u32 i{ 0 }; i < engineShader::count; ++i)
    {
        auto& file = engineShaderFiles[i];

        fullPath = shadersSourcePath;
		fullPath += file.info.fileName;

        if (!std::filesystem::exists(fullPath)) return false;

        dxcCompiledShader compiledShader{ compiler.compile(file.info, fullPath) };

        if (compiledShader.byteCode && compiledShader.byteCode->GetBufferPointer() && compiledShader.byteCode->GetBufferSize())
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
