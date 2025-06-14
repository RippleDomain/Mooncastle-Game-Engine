#include "Common.h"
#include "CommonHeaders.h"
#include "..\Mooncastle\Components\Script.h"

#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif

#include <Windows.h>

using namespace mooncastle;

namespace
{
	HMODULE gameCodeDll{ nullptr };

	using _get_script_creator = mooncastle::script::detail::script_creator(*)(size_t);
	_get_script_creator getScriptCreator{ nullptr };

	using _get_script_names = LPSAFEARRAY(*)(void);
	_get_script_names getScriptNames{ nullptr };
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