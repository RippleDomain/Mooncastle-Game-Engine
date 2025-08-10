#include "Common.h"
#include "CommonHeaders.h"
#include "Components/Script.h"
#include "Graphics/Renderer.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Content/ContentToEngine.h"
#include "ShaderCompilation.h"
#include "../ContentTools/ToolsCommon.h"
#include "Utilities/IOStream.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <atlsafe.h>

using namespace mooncastle;

namespace
{
	HMODULE gameCodeDll{ nullptr };

	using _get_script_creator = mooncastle::script::detail::script_creator(*)(size_t);
	_get_script_creator getScriptCreator{ nullptr };

	using _get_script_names = LPSAFEARRAY(*)(void);
	_get_script_names getScriptNames{ nullptr };

	utl::vector<graphics::renderSurface> surfaces;

	struct shaderData
	{
		u32 type;
		u32 codeSize;
		u32 byteCodeSize;
		u32 errorsSize;
		u32 assemblySize;
		u32 hashSize;
		u8* code;
		u8* byteCodeErrorAssemblyHash;
		const char* functionName;
		const char* extraArgs;
	};

	struct shaderGroupData
	{
		u32 type;
		u32 count;
		u32 dataSize;
		u8* data;
	};
}

EDITOR_INTERFACE u32 LoadGameCodeDll(const char* dllPath) 
{
	if (gameCodeDll) return FALSE;

	gameCodeDll = LoadLibraryA(dllPath);
	assert(gameCodeDll);

	getScriptCreator = (_get_script_creator)GetProcAddress(gameCodeDll, "getScriptCreator");
	getScriptNames = (_get_script_names)GetProcAddress(gameCodeDll, "getScriptNames");

	return (gameCodeDll && getScriptNames && getScriptCreator) ? TRUE : FALSE;
}

EDITOR_INTERFACE u32 UnloadGameCodeDll()
{
	if (!gameCodeDll) return FALSE;

	assert(gameCodeDll);

	[[maybe_unused]] int result{ FreeLibrary(gameCodeDll) };
	assert(result);

	gameCodeDll = nullptr;

	return TRUE;
}

EDITOR_INTERFACE script::detail::script_creator GetScriptCreator(const char* name)
{
	return (gameCodeDll && getScriptCreator) ? getScriptCreator(script::detail::string_hash()(name)) : nullptr;
}

EDITOR_INTERFACE LPSAFEARRAY GetScriptNames(const char* name)
{
	return (gameCodeDll && getScriptNames) ? getScriptNames() : nullptr;
}

EDITOR_INTERFACE u32 CreateRenderSurface(HWND host, i32 width, i32 height)
{
	assert(host);
	platform::windowInitInfo info{ nullptr, host, nullptr, 0, 0, width, height };
	graphics::renderSurface surface{ platform::createWindow(&info), {} };
	assert(surface.window.isValid());
	surfaces.emplace_back(surface);

	return (u32)surfaces.size() - 1;
}

EDITOR_INTERFACE void RemoveRenderSurface(u32 id)
{
	assert(id < surfaces.size());
	platform::removeWindow(surfaces[id].window.getId());
}

EDITOR_INTERFACE HWND GetWindowHandle(u32 id)
{
	assert(id < surfaces.size());
	return (HWND)surfaces[id].window.handle();
}

EDITOR_INTERFACE void ResizeRenderSurface(u32 id)
{
	assert(id < surfaces.size());
	surfaces[id].window.resize(0, 0);
}

EDITOR_INTERFACE id::idType AddShaderGroup(shaderGroupData* data)
{
    assert(data && data->type < graphics::shaderType::count && data->count && data->dataSize && data->data);

    const u32 count{ data->count };

    utl::blobStreamReader blob{ data->data };
    const u32 *const keys{ (const u32*)blob.getPosition() };
	blob.skip(count * sizeof(u32)); //Skips the keys.

    const u8** shaderPtrs{ (const u8**)alloca(count * sizeof(u8*)) };

    for (u32 i{ 0 }; i < count; ++i)
    {
        const u32 blockSize{ sizeof(u64) + content::compiledShader::hashLength + *(u32*)blob.getPosition() };
        shaderPtrs[i] = blob.getPosition();
        blob.skip(blockSize);
    }

    assert(blob.getPosition() == (data->data + data->dataSize));

    return content::addShaderGroup(shaderPtrs, count, keys);
}

EDITOR_INTERFACE void RemoveShaderGroup(id::idType id)
{
    content::destroyShaderGroup(id);
}

EDITOR_INTERFACE u32 CompileShader(shaderData* data)
{
    assert(data && data->code && data->codeSize && data->functionName);

    shaderFileInfo info{};
    info.function = data->functionName;
    info.type = (graphics::shaderType::type)data->type;

    utl::vector<std::string> extraArgs{ split(data->extraArgs, ';') };
    utl::vector<std::wstring> wExtraArgs{};

    for (const auto& str : extraArgs)
    {
        wExtraArgs.emplace_back(toWString(str.c_str()));
    }

    std::unique_ptr<u8[]> compiledShader{ compileShader(info, data->code, data->codeSize, wExtraArgs, true) };

    if (!compiledShader) return FALSE;

    u64 bufferSize{ 0 };

    {
        utl::blobStreamReader blob{ compiledShader.get() };
        data->byteCodeSize = (u32)blob.read<u64>();
        data->hashSize = content::compiledShader::hashLength;

        blob.skip(data->hashSize + data->byteCodeSize);

        data->errorsSize = (u32)blob.read<u64>();
        data->assemblySize = (u32)blob.read<u64>();

        bufferSize = data->byteCodeSize + data->hashSize + data->errorsSize + data->assemblySize;
    }

    assert(bufferSize);

    data->byteCodeErrorAssemblyHash = (u8*)CoTaskMemAlloc(bufferSize);
    assert(data->byteCodeErrorAssemblyHash);

    {
        utl::blobStreamReader blob{ compiledShader.get() };

        blob.skip(sizeof(u64)); //Skips the size of the byte-code buffer.
        blob.read(&data->byteCodeErrorAssemblyHash[bufferSize - data->hashSize], data->hashSize);
        blob.read(data->byteCodeErrorAssemblyHash, data->byteCodeSize);
        blob.skip(2 * sizeof(u64)); //Skips the size of the error and assembly buffers.
        blob.read(&data->byteCodeErrorAssemblyHash[data->byteCodeSize], data->errorsSize + data->assemblySize);
    }

    return TRUE;
}