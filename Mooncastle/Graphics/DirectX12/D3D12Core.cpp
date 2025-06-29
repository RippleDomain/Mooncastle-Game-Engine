#include "D3D12Core.h"

using namespace Microsoft::WRL;

namespace mooncastle::graphics::d3D12::core 
{
    namespace 
    {
        ID3D12Device8* mainDevice{ nullptr };
        IDXGIFactory7* dxgiFactory{ nullptr };

        constexpr D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

        bool failedInit()
        {
            shutdown();
            return false;
        }

        //TODO: Can add monitor and resolution checks, give the option to pick a specific adapter, etc.
        //Get the most performant adapter that supports the minimum feature level.
        IDXGIAdapter4* determineMainAdapter()
        {
            IDXGIAdapter4* adapter{ nullptr };

            //Get adapters in descending order of performance.
            for (u32 i{ 0 }; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                //Pick the first adapter that supports the minimum feature level.
                if (SUCCEEDED(D3D12CreateDevice(adapter, minFeatureLevel, __uuidof(ID3D12Device), nullptr)))
                {
                    return adapter;
                }

                release(adapter);
            }

            return nullptr;
        }

        D3D_FEATURE_LEVEL getMaxFeatureLevel(IDXGIAdapter4* adapter)
        {
            constexpr D3D_FEATURE_LEVEL featureLevels[4]{
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_12_1,
            };

            D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo{};
            featureLevelInfo.NumFeatureLevels = _countof(featureLevels);
            featureLevelInfo.pFeatureLevelsRequested = featureLevels;

            ComPtr<ID3D12Device> device;
            DXCall(D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&device)));
            DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo)));

            return featureLevelInfo.MaxSupportedFeatureLevel;
        }
    }

    bool initialize()
    {
        //Determine what is the maxmimum feature level that is supported.
        //Create a ID3D12Device.

        if (mainDevice) shutdown();

        u32 dxgiFactoryFlags{ 0 };
#ifdef _DEBUG
        //Requires "Graphics Tools" optional feature.
        {
            ComPtr<ID3D12Debug3> debugInterface;
            DXCall(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
            debugInterface->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif

        HRESULT hr{ S_OK };
        DXCall(hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        if (FAILED(hr)) return failedInit();

        //Determine which adapter (for example, GPU) to use.
        ComPtr<IDXGIAdapter4> mainAdapter;
        mainAdapter.Attach(determineMainAdapter());

        if (!mainAdapter) return failedInit();

        D3D_FEATURE_LEVEL maxFeatureLevel{ getMaxFeatureLevel(mainAdapter.Get()) };
        assert(maxFeatureLevel >= minFeatureLevel);

        if (maxFeatureLevel < minFeatureLevel) return failedInit();

        DXCall(hr = D3D12CreateDevice(mainAdapter.Get(), maxFeatureLevel, IID_PPV_ARGS(&mainDevice)));
        if (FAILED(hr)) return failedInit();

        NAME_D3D12_OBJECT(mainDevice, L"Main D3D12 Device");

#ifdef _DEBUG
        {
            ComPtr<ID3D12InfoQueue> infoQueue;
            DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        }
#endif

        return true;
    }

    void shutdown()
    {
        release(dxgiFactory);

#ifdef _DEBUG
        {
            {
                ComPtr<ID3D12InfoQueue> infoQueue;
                DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
            }

            ComPtr<ID3D12DebugDevice2> debugDevice;
            DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&debugDevice)));
            release(mainDevice);
            DXCall(debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
        }
#endif

        release(mainDevice);
    }
}