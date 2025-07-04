#pragma once

#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace mooncastle::graphics::d3D12 
{
    class D3D12Surface
    {
    public:
        constexpr static u32 bufferCount{ 3 };

        explicit D3D12Surface(platform::window window) : window(window)
        {
            assert(window.handle());
        }

#if USE_STL_VECTOR
        DISABLE_COPY(D3D12Surface);
        constexpr D3D12Surface(D3D12Surface&& o)
            : swapChain{ o.swapChain }, window{ o.window }, currentBBIndex{ o.currentBBIndex }
            , viewport{ o.viewport }, scissorRect{ o.scissorRect }, allowTearing{ o.allowTearing }, presentFlags{ o.presentFlags }
        {
            for (u32 i{ 0 }; i < bufferCount; ++i)
            {
                targetData[i].resource = o.targetData[i].resource;
                targetData[i].rtv = o.targetData[i].rtv;
            }

            o.reset();
        }

        constexpr D3D12Surface& operator=(D3D12Surface&& o)
        {
            assert(this != &o);

            if (this != &o)
            {
                release();
                move(o);
            }

            return *this;
        }
#else
        DISABLE_COPY_AND_MOVE(D3D12Surface);
#endif

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

#if USE_STL_VECTOR
        constexpr void reset()
        {
            swapChain = nullptr;

            for (u32 i{ 0 }; i < bufferCount; ++i)
            {
                targetData[i] = {};
            }

            window = {};
            currentBBIndex = 0;
            allowTearing = 0;
            presentFlags = 0;
            viewport = {};
            scissorRect = {};
        }

        constexpr void move(D3D12Surface& o)
        {
            swapChain = o.swapChain;
            
            for (u32 i{ 0 }; i < bufferCount; ++i)
            {
                targetData[i] = o.targetData[i];
            }

            window = o.window;
            currentBBIndex = o.currentBBIndex;
            allowTearing = o.allowTearing;
            presentFlags = o.presentFlags;
            viewport = o.viewport;
            scissorRect = o.scissorRect;

            o.reset();
        }
#endif USE_STL_VECTOR

        struct renderTargetData
        {
            ID3D12Resource* resource{ nullptr };
            descriptorHandle rtv{};
        };

        IDXGISwapChain4* swapChain{ nullptr };
        renderTargetData targetData[bufferCount]{};
        platform::window window{};
        mutable u32      currentBBIndex{ 0 };
        u32              allowTearing{ 0 };
        u32              presentFlags{ 0 };
        D3D12_VIEWPORT   viewport{};
        D3D12_RECT       scissorRect{};
    };
}