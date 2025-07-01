#pragma once

#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace mooncastle::graphics::d3D12 
{
    class D3D12Surface
    {
    public:
        explicit D3D12Surface(platform::window window) : window(window)
        {
            assert(window.handle());
        }

        ~D3D12Surface() 
        { 
            release(); 
        }

        void createSwapChain(IDXGIFactory7* factory, ID3D12CommandQueue* cmdQueue, DXGI_FORMAT format);
        void present() const;
        void resize();

        constexpr u32 getWidth() const { return (u32)viewport.Width; }
        constexpr u32 getHeight() const { return (u32)viewport.Height; }
        constexpr ID3D12Resource* const getBackBuffer() const { return targetData[currentBBIndex].resource; }
        constexpr D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const { return targetData[currentBBIndex].rtv.cpu; }
        constexpr const D3D12_VIEWPORT& getViewport() const { return viewport; }
        constexpr const D3D12_RECT& getScissorRect() const { return scissorRect; }

    private:
        void release();
        void finalize();

        struct renderTargetData
        {
            ID3D12Resource* resource{ nullptr };
            descriptorHandle rtv{};
        };

        IDXGISwapChain4* swapChain{ nullptr };
        renderTargetData targetData[frameBufferCount]{};
        platform::window window{};
        mutable u32      currentBBIndex{ 0 };
        D3D12_VIEWPORT   viewport{};
        D3D12_RECT       scissorRect{};
    };
}