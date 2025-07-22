#pragma once

#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace mooncastle::graphics::d3D12
{
    struct D3D12FrameInfo
    {
        u32 surfaceWidth{};
        u32 surfaceHeight{};
    };
}

namespace mooncastle::graphics::d3D12::core 
{
	bool initialize();
	void shutdown();

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

    [[nodiscard]] ID3D12Device *const device();

    [[nodiscard]] descriptorHeap& getRTVHeap();
    [[nodiscard]] descriptorHeap& getDSVHeap();
    [[nodiscard]] descriptorHeap& getSRVHeap();
    [[nodiscard]] descriptorHeap& getUAVHeap();

    [[nodiscard]] constantBuffer& getConstantBuffer();

    [[nodiscard]] u32 getCurrentFrameIndex();
    void setDeferredReleasesFlag();

    [[nodiscard]] surface createSurface(platform::window window);
    void removeSurface(surfaceId id);
    void resizeSurface(surfaceId id, u32, u32);
    [[nodiscard]] u32 surfaceWidth(surfaceId id);
    [[nodiscard]] u32 surfaceHeight(surfaceId id);
    void renderSurface(surfaceId id);
}