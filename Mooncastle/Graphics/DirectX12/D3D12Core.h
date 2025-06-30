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

    namespace detail 
    {
        void deferredRelease(IUnknown* resource);
    }

    template<typename T>
    constexpr void deferredRelease(T*& resource)
    {
        if (resource)
        {
            detail::deferredRelease(resource);
            resource = nullptr;
        }
    }

    ID3D12Device *const device();
    u32 currentFrameIndex();
    void setDeferredReleasesFlag();
}