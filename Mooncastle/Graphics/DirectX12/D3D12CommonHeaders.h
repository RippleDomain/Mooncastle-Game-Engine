#pragma once

#include "CommonHeaders.h"
#include "Graphics\Renderer.h"

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

//Assert that COM call to D3D API succeeded
#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)                                              \
if(FAILED(x))                                                  \
{                                                              \
    char lineNumber[32];                                       \
    sprintf_s(lineNumber, "%u", __LINE__);                     \
    OutputDebugStringA("Error in: ");                          \
    OutputDebugStringA(__FILE__);                              \
    OutputDebugStringA("\nLine: ");                            \
    OutputDebugStringA(lineNumber);                            \
    OutputDebugStringA("\n");                                  \
    OutputDebugStringA(#x);                                    \
    OutputDebugStringA("\n");                                  \
    __debugbreak();                                            \
}
#endif
#else
#ifndef DXCall
#define DXCall(x) x
#endif
#endif

#ifdef _DEBUG
//Sets the name of the COM object and outputs a debug string in the output panel.
#define NAME_D3D12_OBJECT(obj, name) obj->SetName(name); OutputDebugString(L"::D3D12 Object Created: "); OutputDebugString(name); OutputDebugString(L"\n");
#else
#define NAME_D3D12_OBJECT(x, name)
#endif