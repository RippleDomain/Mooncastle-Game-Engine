#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::d3DX
{
    constexpr struct
    {
        const D3D12_HEAP_PROPERTIES defaultHeap
        {
            D3D12_HEAP_TYPE_DEFAULT,             //Type
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,     //CPUPageProperty
            D3D12_MEMORY_POOL_UNKNOWN,           //MemoryPoolPreference
            0,                                   //CreationNodeMask
            0                                    //VisibleNodeMask
        };
    } heapProperties;
}