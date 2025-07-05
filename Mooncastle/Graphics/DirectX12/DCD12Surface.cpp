#include "D3D12Surface.h"
#include "D3D12Core.h"

namespace mooncastle::graphics::d3D12
{
    namespace
    {
        constexpr DXGI_FORMAT toNonSrgb(DXGI_FORMAT format)
        {
            if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) return DXGI_FORMAT_R8G8B8A8_UNORM;

            return format;
        }
    }

    void D3D12Surface::createSwapChain(IDXGIFactory7* factory, ID3D12CommandQueue* cmdQueue, DXGI_FORMAT format /*= defaultBackBufferFormat*/)
    {
        assert(factory && cmdQueue);

        release();

        if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(u32))) && allowTearing)
        {
            presentFlags = DXGI_PRESENT_ALLOW_TEARING;
        }

        this->format = format;

        //allowTearing = presentFlags = 0;

        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.BufferCount = bufferCount;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        desc.Format = toNonSrgb(format);
        desc.Height = window.height();
        desc.Width = window.width();
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Scaling = DXGI_SCALING_STRETCH; desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Stereo = false;

        IDXGISwapChain1* newSwapChain;
        HWND hwnd{ (HWND)window.handle() };

        DXCall(factory->CreateSwapChainForHwnd(cmdQueue, hwnd, &desc, nullptr, nullptr, &newSwapChain));
        DXCall(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
        DXCall(newSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));

        core::release(newSwapChain);
        currentBBIndex = swapChain->GetCurrentBackBufferIndex();

        for (u32 i{ 0 }; i < bufferCount; ++i)
        {
            targetData[i].rtv = core::getRTVHeap().allocate();
        }

        finalize();
    }

    void D3D12Surface::present() const
    {
        assert(swapChain);
        DXCall(swapChain->Present(0, presentFlags));
        currentBBIndex = swapChain->GetCurrentBackBufferIndex();
    }

    void D3D12Surface::resize()
    {

    }

    void D3D12Surface::finalize()
    {
        //Create RTVs for back-buffers.
        for (u32 i{ 0 }; i < bufferCount; ++i)
        {
            renderTargetData& data{ targetData[i] };
            assert(!data.resource);
            DXCall(swapChain->GetBuffer(i, IID_PPV_ARGS(&data.resource)));
            D3D12_RENDER_TARGET_VIEW_DESC desc{};
            desc.Format = format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            core::device()->CreateRenderTargetView(data.resource, &desc, data.rtv.cpu);
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        DXCall(swapChain->GetDesc(&desc));
        const u32 width{ desc.BufferDesc.Width };
        const u32 height{ desc.BufferDesc.Height };
        assert(window.width() == width && window.height() == height);

        viewport.TopLeftX = 0.f;
        viewport.TopLeftY = 0.f;
        viewport.Width = (float)width;
        viewport.Height = (float)height;
        viewport.MinDepth = 0.f;
        viewport.MaxDepth = 1.f;

        scissorRect = { 0, 0, (i32)width, (i32)height };
    }

    void D3D12Surface::release()
    {
        for (u32 i{ 0 }; i < bufferCount; ++i)
        {
            renderTargetData& data{ targetData[i] };
            core::release(data.resource);
            core::getRTVHeap().free(data.rtv);
        }

        core::release(swapChain);
    }
}