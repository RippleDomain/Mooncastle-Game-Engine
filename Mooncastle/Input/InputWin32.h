#pragma once

#ifdef _WIN64

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

namespace mooncastle::input
{
	HRESULT processInputMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
}

#endif