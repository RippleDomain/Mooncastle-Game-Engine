#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::core 
{
	bool initialize();
	void shutdown();
	void render();

    template<typename T>
    constexpr void release(T*& resource)
    {
        if (resource)
        {
            resource->Release();
            resource = nullptr;
        }
    }
}