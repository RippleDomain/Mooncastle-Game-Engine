#pragma once

#include "CommonHeaders.h"
#include "Graphics\Renderer.h"
#include "Platform\Window.h"

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace mooncastle::graphics::d3D12 
{
    constexpr u32 frameBufferCount{ 3 };

    using ID3D12Device = ID3D12Device8;
    using ID3D12GraphicsCommandList = ID3D12GraphicsCommandList6;
}

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
// The indexed variant will include the index in the name of the object
#define NAME_D3D12_OBJECT_INDEXED(obj, n, name)                           \
{                                                                         \
    wchar_t fullName[128];                                                \
    if (swprintf_s(fullName, L"%s[%u]", name, n) > 0)                     \
    {                                                                     \
        obj->SetName(fullName);                                           \
        OutputDebugString(L"::D3D12 Object Created: ");                   \
        OutputDebugString(fullName);                                      \
        OutputDebugString(L"\n");                                         \
    }                                                                     \
}
#else
#define NAME_D3D12_OBJECT(x, name)
#define NAME_D3D12_OBJECT_INDEXED(obj, x, name)
#endif

#include "D3D12Resources.h"
#include "D3D12Helpers.h"