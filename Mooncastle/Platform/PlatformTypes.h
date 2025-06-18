#pragma once
#include "CommonHeaders.h"

#ifdef _WIN64
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif

#include <Windows.h>

namespace mooncastle::platform {

	using windowProc = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);
	using windowHandle = HWND;

	struct windowInitInfo
	{
		windowProc        callback{ nullptr };
		windowHandle      parent{ nullptr };
		const wchar_t*    caption{ nullptr };
		i32               left{ 0 };
		i32               top{ 0 };
		i32               width{ 1920 };
		i32               height{ 1080 };
	};

}

#endif