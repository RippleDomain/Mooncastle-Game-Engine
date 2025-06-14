#include "Common.h"
#include "CommonHeaders.h"

#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif

#include <Windows.h>

using namespace mooncastle;

namespace
{
	HMODULE gameCodeDll{ nullptr };
}

EDITOR_INTERFACE u32 LoadGameCodeDll(const char* dllPath) 
{
	if (gameCodeDll) return FALSE;

	gameCodeDll = LoadLibraryA(dllPath);
	assert(gameCodeDll);

	return gameCodeDll ? TRUE : FALSE;
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