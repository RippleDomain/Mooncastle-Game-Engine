#include "ShaderCompilation.h"

#include <wrl.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#include "Graphics/Renderer.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStream.h"

#include <fstream>
#include <filesystem>

using namespace mooncastle;
using namespace Microsoft::WRL;

// Assert that COM call to D3D API succeeded
#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)                                           \
if(FAILED(x))												\
{															\
    char lineNumber[32];                                    \
    sprintf_s(lineNumber, "%u", __LINE__);                  \
    OutputDebugStringA("Error in: ");                       \
    OutputDebugStringA(__FILE__);                           \
    OutputDebugStringA("\nLine: ");                         \
    OutputDebugStringA(lineNumber);                         \
    OutputDebugStringA("\n");                               \
    OutputDebugStringA(#x);                                 \
    OutputDebugStringA("\n");                               \
    __debugbreak();                                         \
}
#endif
#else
#ifndef DXCall
#define DXCall(x) x
#endif
#endif

namespace
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

	struct engineShaderInfo
	{
		engineShader::id id;
		shaderFileInfo info;
	};

	constexpr const char* shadersSourcePath{ "../../Mooncastle/Graphics/DirectX12/Shaders/" };

	constexpr engineShaderInfo engineShaderFiles[]
	{
		{engineShader::fullscreenTriangleVS, {"FullScreenTriangle.hlsl", "FullScreenTriangleVS", graphics::shaderType::vertex}},
		{engineShader::postProcessPS,        {"PostProcess.hlsl", "PostProcessPS", graphics::shaderType::pixel}},
		{engineShader::gridFrustumsCS,       {"GridFrustums.hlsl", "ComputeGridFrustumsCS", graphics::shaderType::compute}},
		{engineShader::lightCullingCS,       {"CullLights.hlsl", "CullLightsCS", graphics::shaderType::compute}}
	};

	static_assert(_countof(engineShaderFiles) == engineShader::count);

	struct dxcCompiledShader
	{
		ComPtr<IDxcBlob>		byteCode;
		ComPtr<IDxcBlobUtf8>    errors;
		ComPtr<IDxcBlobUtf8>    assembly;
		DxcShaderHash			hash;
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

		dxcCompiledShader compile(u8* data, u32 dataSize, graphics::shaderType::type type, const char* function, utl::vector<std::wstring>& extraArgs)
		{
			assert(compiler && utils && includeHandler);
			assert(data && dataSize && function);
			assert(type < graphics::shaderType::count);

			HRESULT hResult{ S_OK };

			ComPtr<IDxcBlobEncoding> sourceBlob{ nullptr };
			DXCall(hResult = utils->CreateBlob(data, dataSize, 0, &sourceBlob));

			if (FAILED(hResult)) return {};

			assert(sourceBlob && sourceBlob->GetBufferSize());

			shaderFileInfo info{};
			info.function = function;
			info.type = type;

			OutputDebugStringA("Compiling ");
			OutputDebugStringA(function);
			OutputDebugStringA("\n");

			return compile(sourceBlob.Get(), getArgs(info, extraArgs));
		}

		dxcCompiledShader compile(shaderFileInfo info, std::filesystem::path fullPath, utl::vector<std::wstring>& extraArgs)
		{
			assert(compiler && utils && includeHandler);
			HRESULT hr{ S_OK };

			ComPtr<IDxcBlobEncoding> sourceBlob{ nullptr };
			DXCall(hr = utils->LoadFile(fullPath.c_str(), nullptr, &sourceBlob));
			if (FAILED(hr)) return {};

			assert(sourceBlob && sourceBlob->GetBufferSize());

			OutputDebugStringA("Compiling ");
			OutputDebugStringA(info.fileName);
			OutputDebugStringA(" : ");
			OutputDebugStringA(info.function);
			OutputDebugStringA("\n");

			return compile(sourceBlob.Get(), getArgs(info, extraArgs));
		}

		dxcCompiledShader compile(IDxcBlobEncoding* sourceBlob, utl::vector<std::wstring> compilerArgs)
		{
			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = sourceBlob->GetBufferPointer();
			buffer.Size = sourceBlob->GetBufferSize();

			utl::vector<LPCWSTR> args;

			for (const auto& arg : compilerArgs)
			{
				args.emplace_back(arg.c_str());
			}

			HRESULT hr{ S_OK };
			ComPtr<IDxcResult> results{ nullptr };
			DXCall(hr = compiler->Compile(&buffer, args.data(), (u32)args.size(), includeHandler.Get(), IID_PPV_ARGS(&results)));
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

			dxcCompiledShader result{ shader.Detach(), errors.Detach(), disassembly.Detach() };
			memcpy(&result.hash.HashDigest[0], &hashBuffer->HashDigest[0], _countof(hashBuffer->HashDigest));

			return result;
		}

	private:
		utl::vector<std::wstring> getArgs(const shaderFileInfo& info, mooncastle::utl::vector<std::wstring>& extraArgs)
		{
			utl::vector<std::wstring> args{};

			if (info.fileName) args.emplace_back(toWString(info.fileName));			//Optional shader source file name for reporting possible errors.

			args.emplace_back(L"-E");												//Entry function.
			args.emplace_back(toWString(info.function));
			args.emplace_back(L"-T");												//Target profile.
			args.emplace_back(toWString(profileStrings[(u32)info.type]));
			args.emplace_back(L"-I");												//Include path.
			args.emplace_back(toWString(shadersSourcePath));
			args.emplace_back(L"-enable-16bit-types");
			args.emplace_back(DXC_ARG_ALL_RESOURCES_BOUND);

#if _DEBUG
			args.emplace_back(DXC_ARG_DEBUG);
			args.emplace_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
			args.emplace_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

			args.emplace_back(DXC_ARG_WARNINGS_ARE_ERRORS);					//Treats warnings as errors.
			args.emplace_back(L"-Qstrip_reflect");							//Strips reflections into a separate blob.
			args.emplace_back(L"-Qstrip_debug");							//Strips debug information into a separate blob.

			for (const auto& arg : extraArgs)
			{
				args.emplace_back(arg.c_str());
			}

			return args;
		}

		constexpr static const char* profileStrings[]{ "vs_6_6", "hs_6_6", "ds_6_6", "gs_6_6", "ps_6_6", "cs_6_6", "as_6_6", "ms_6_6" };
		static_assert(_countof(profileStrings) == graphics::shaderType::count);

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

		for (const auto& entry : std::filesystem::directory_iterator{ shadersSourcePath })
		{
			if (entry.last_write_time() > shadersCompilationTime)
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

		for (const auto& shader : shaders)
		{
			void *const byteCode{ shader.byteCode->GetBufferPointer() };
			const u64 byteCodeLength{ shader.byteCode->GetBufferSize() };

			file.write((char*)&byteCodeLength, sizeof(byteCodeLength));
			file.write((char*)&shader.hash.HashDigest[0], _countof(shader.hash.HashDigest));
			file.write((char*)byteCode, byteCodeLength);
		}

		file.close();

		return true;
	}

	std::unique_ptr<u8[]> packCompiledShader(dxcCompiledShader compiledShader, bool includeErrorsAndDisassembly)
	{
		if (compiledShader.byteCode && compiledShader.byteCode->GetBufferPointer() && compiledShader.byteCode->GetBufferSize())
		{
			static_assert(content::compiledShader::hashLength == _countof(DxcShaderHash::HashDigest));

			const u64 extraSize{ includeErrorsAndDisassembly ? sizeof(u64) + sizeof(u64) + compiledShader.errors->GetStringLength() + compiledShader.assembly->GetStringLength() : 0 };
			const u64 bufferSize{ sizeof(u64) + content::compiledShader::hashLength + compiledShader.byteCode->GetBufferSize() + extraSize };

			std::unique_ptr<u8[]> buffer{ std::make_unique<u8[]>(bufferSize) };
			utl::blobStreamWriter blob{ buffer.get(), bufferSize };

			blob.write(compiledShader.byteCode->GetBufferSize());
			blob.write(compiledShader.hash.HashDigest, content::compiledShader::hashLength);
			blob.write((u8*)compiledShader.byteCode->GetBufferPointer(), compiledShader.byteCode->GetBufferSize());

			if (includeErrorsAndDisassembly)
			{
				blob.write(compiledShader.errors->GetStringLength());
				blob.write(compiledShader.assembly->GetStringLength());
				blob.write(compiledShader.errors->GetStringPointer(), compiledShader.errors->GetStringLength());
				blob.write(compiledShader.assembly->GetStringPointer(), compiledShader.assembly->GetStringLength());
			}

			assert(blob.getOffset() == bufferSize);

			return buffer;
		}

		return {};
	}
}

std::unique_ptr<u8[]> compileShader(shaderFileInfo info, u8* code, u32 codeSize, 
	utl::vector<std::wstring>& extraArgs, bool includeErrorsAndDisassembly/*=false*/)
{
	assert(!info.fileName && info.function && code && codeSize);
	return packCompiledShader(shaderCompiler{}.compile(code, codeSize, info.type, info.function, extraArgs), includeErrorsAndDisassembly);
}

std::unique_ptr<u8[]> compileShader(shaderFileInfo info, const char* filePath, 
	utl::vector<std::wstring>& extraArgs, bool includeErrorsAndDisassembly/*=false*/)
{
	std::filesystem::path fullPath{ filePath };
	fullPath += info.fileName;
	if (!std::filesystem::exists(fullPath)) return {};

	return packCompiledShader(shaderCompiler{}.compile(info, fullPath, extraArgs), includeErrorsAndDisassembly);
}

bool compileShaders()
{
	if (compiledShadersAreUpToDate()) return true;

	shaderCompiler compiler{};
	utl::vector<dxcCompiledShader> shaders;
	std::filesystem::path fullPath{};

	for (u32 i{ 0 }; i < engineShader::count; ++i)
	{
		auto& file = engineShaderFiles[i];

		fullPath = shadersSourcePath;
		fullPath += file.info.fileName;

		if (!std::filesystem::exists(fullPath)) return false;

		utl::vector<std::wstring> extraArgs{};

		if (file.id == engineShader::gridFrustumsCS || file.id == engineShader::lightCullingCS)
		{
			extraArgs.emplace_back(L"-D");
			extraArgs.emplace_back(L"TILE_SIZE=32");
		}

		dxcCompiledShader compiledShader{ compiler.compile(file.info, fullPath, extraArgs) };

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