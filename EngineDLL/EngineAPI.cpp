#include "Common.h"
#include "CommonHeaders.h"
#include "Components/Script.h"
#include "Graphics/Renderer.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"

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

	int result = FreeLibrary(gameCodeDll);
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