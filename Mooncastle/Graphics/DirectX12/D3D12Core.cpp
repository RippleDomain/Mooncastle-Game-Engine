#include "D3D12Core.h"
#include "D3D12Resources.h"
#include "D3D12Surface.h"

using namespace Microsoft::WRL;

namespace mooncastle::graphics::d3D12::core 
{
    namespace 
    {
        class d3D12Command
        {
        public:
            d3D12Command() = default;

            DISABLE_COPY_AND_MOVE(d3D12Command);

            explicit d3D12Command(ID3D12Device8 *const device, D3D12_COMMAND_LIST_TYPE type)
            {
                HRESULT hr{ S_OK };
                D3D12_COMMAND_QUEUE_DESC desc{};

                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 0;
                desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
                desc.Type = type;

                DXCall(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
                if (FAILED(hr)) goto error;

                NAME_D3D12_OBJECT(commandQueue,
                    type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
                    L"GFX Command Queue" :
                    type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
                    L"Compute Command Queue" : L"Command Queue");

                for (u32 i{ 0 }; i < frameBufferCount; ++i)
                {
                    commandFrame& frame{ commandFrames[i] };
                    DXCall(hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.commandAllocator)));
                    if (FAILED(hr)) goto error;

                    NAME_D3D12_OBJECT_INDEXED(frame.commandAllocator, i,
                        type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
                        L"GFX Command Allocator" :
                        type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
                        L"Compute Command Allocator" : L"Command Allocator");
                }

                DXCall(hr = device->CreateCommandList(0, type, commandFrames[0].commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));
                if (FAILED(hr)) goto error;

                DXCall(commandList->Close());
                NAME_D3D12_OBJECT(commandList,
                    type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
                    L"GFX Command List" :
                    type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
                    L"Compute Command List" : L"Command List");

                DXCall(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
                if (FAILED(hr)) goto error;

                NAME_D3D12_OBJECT(fence, L"D3D12 Fence");
                fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
                assert(fenceEvent);

                return;

            error:
                release();
            }

            ~d3D12Command()
            {
                assert(!commandQueue && !commandList && !fence);
            }

            //Waits for the current frame to be signalled and resets the command list and the allocator.
            void beginFrame()
            {
                commandFrame& frame{ commandFrames[frameIndex] };
                frame.wait(fenceEvent, fence);

                DXCall(frame.commandAllocator->Reset());
                DXCall(commandList->Reset(frame.commandAllocator, nullptr));
            }

            //Signals the frame with the new fence value.
            void endFrame()
            {
                DXCall(commandList->Close());
                ID3D12CommandList *const cmdLists[]{ commandList };
                commandQueue->ExecuteCommandLists(_countof(cmdLists), &cmdLists[0]);

                u64& fenceValueRef{ fenceValue };
                ++fenceValueRef;
                commandFrame& frame{ commandFrames[frameIndex] };
                frame.fenceValue = fenceValueRef;

                commandQueue->Signal(fence, fenceValueRef);

                frameIndex = (frameIndex + 1) % frameBufferCount;
            }

            void flush()
            {
                for (u32 i{ 0 }; i < frameBufferCount; ++i)
                {
                    commandFrames[i].wait(fenceEvent, fence);
                }

                frameIndex = 0;
            }

            void release()
            {
                flush();
                core::release(fence);
                fenceValue = 0;

                CloseHandle(fenceEvent);
                fenceEvent = nullptr;
                core::release(commandQueue);
                core::release(commandList);

                for (u32 i{ 0 }; i < frameBufferCount; ++i)
                {
                    commandFrames[i].release();
                }
            }

            constexpr ID3D12CommandQueue *const getCommandQueue() const { return commandQueue; }
            constexpr ID3D12GraphicsCommandList6 *const getCommandList() const { return commandList; }
            constexpr u32 getFrameIndex() const { return frameIndex; }

        private:
            struct commandFrame
            {
                ID3D12CommandAllocator* commandAllocator{ nullptr };
                u64                     fenceValue{ 0 };

                void wait(HANDLE fenceEvent, ID3D12Fence1* fence)
                {
                    assert(fence && fenceEvent);

                    //If the current fence value is still less than "fenceValue" the GPU has not finished executing the command lists.
                    if (fence->GetCompletedValue() < fenceValue)
                    {
                        //Make the fence create an event which is signaled once the fence's current value equals "fenceValue"
                        DXCall(fence->SetEventOnCompletion(fenceValue, fenceEvent));

                        //Wait until the fence has triggered the event, which means the command has finished executing.
                        WaitForSingleObject(fenceEvent, INFINITE);
                    }
                }

                void release()
                {
                    core::release(commandAllocator);
                    fenceValue = 0;
                }
            };

            ID3D12CommandQueue*         commandQueue{ nullptr };
            ID3D12GraphicsCommandList6* commandList{ nullptr };
            ID3D12Fence1*               fence{ nullptr };
            u64                         fenceValue{ 0 };
            HANDLE                      fenceEvent{ nullptr };
            commandFrame                commandFrames[frameBufferCount]{};
            u32                         frameIndex{ 0 };
        };

        ID3D12Device8*            mainDevice{ nullptr };
        IDXGIFactory7*            dxgiFactory{ nullptr };
        d3D12Command              gfxCommand;
        utl::vector<D3D12Surface> surfaces;
        descriptorHeap            rtvDescriptorHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
        descriptorHeap            dsvDescriptorHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
        descriptorHeap            srvDescriptorHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
        descriptorHeap            uavDescriptorHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
        utl::vector<IUnknown*>    deferredReleases[frameBufferCount]{};
        u32                       deferredReleaseFlag[frameBufferCount]{};
        std::mutex                deferredReleasesMutex{};

        constexpr DXGI_FORMAT renderTargetFormat{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
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

        void __declspec(noinline) processDeferredReleases(u32 frameIndex)
        {
            std::lock_guard lock{ deferredReleasesMutex };
            deferredReleaseFlag[frameIndex] = 0;

            rtvDescriptorHeap.processDeferredFree(frameIndex);
            dsvDescriptorHeap.processDeferredFree(frameIndex);
            srvDescriptorHeap.processDeferredFree(frameIndex);
            uavDescriptorHeap.processDeferredFree(frameIndex);
            
            utl::vector<IUnknown*>& resources{ deferredReleases[frameIndex] };

            if (!resources.empty())
            {
                for (auto& resource : resources) release(resource);
                resources.clear();
            }
        }
    }

    namespace detail 
    {
        void deferredRelease(IUnknown* resource)
        {
            const u32 frameIndex{ currentFrameIndex() };
            std::lock_guard lock{ deferredReleasesMutex };
            deferredReleases[frameIndex].push_back(resource);

            setDeferredReleasesFlag();
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

            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) 
            {
                debugInterface->EnableDebugLayer();
            }
            else
            {
                OutputDebugStringA("Warning: D3D12 Debug interface is not available.");
            }

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

#ifdef _DEBUG
        {
            ComPtr<ID3D12InfoQueue> infoQueue;
            DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        }
#endif

        bool result{ true };
        result &= rtvDescriptorHeap.initialize(512, false);
        result &= dsvDescriptorHeap.initialize(512, false);
        result &= srvDescriptorHeap.initialize(4096, true);
        result &= uavDescriptorHeap.initialize(512, false);
        if (!result) return failedInit();

        new (&gfxCommand)d3D12Command(mainDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
        if (!gfxCommand.getCommandQueue()) return failedInit();

        NAME_D3D12_OBJECT(mainDevice, L"Main D3D12 Device");
        NAME_D3D12_OBJECT(rtvDescriptorHeap.getHeap(), L"RTV Descriptor Heap");
        NAME_D3D12_OBJECT(dsvDescriptorHeap.getHeap(), L"DSV Descriptor Heap");
        NAME_D3D12_OBJECT(srvDescriptorHeap.getHeap(), L"SRV Descriptor Heap");
        NAME_D3D12_OBJECT(uavDescriptorHeap.getHeap(), L"UAV Descriptor Heap");

        return true;
    }

    void shutdown()
    {
        gfxCommand.release();

        //We don't call processDeferredReleases at the end because some resources (such as swap chains) can't be released before their depending resources are released.
        for (u32 i{ 0 }; i < frameBufferCount; ++i)
        {
            processDeferredReleases(i);
        }

        release(dxgiFactory);

        rtvDescriptorHeap.processDeferredFree(0);
        dsvDescriptorHeap.processDeferredFree(0);
        srvDescriptorHeap.processDeferredFree(0);
        uavDescriptorHeap.processDeferredFree(0);

        rtvDescriptorHeap.release();
        dsvDescriptorHeap.release();
        srvDescriptorHeap.release();
        uavDescriptorHeap.release();

        processDeferredReleases(0);

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

    ID3D12Device *const device()
    {
        return mainDevice;
    }

    descriptorHeap& getRTVHeap()
    {
        return rtvDescriptorHeap;
    }

    descriptorHeap& getDSVHeap()
    {
        return dsvDescriptorHeap;
    }

    descriptorHeap& getSRVHeap() 
    {
        return srvDescriptorHeap;
    }

    descriptorHeap& getUAVHeap()
    {
        return uavDescriptorHeap;
    }

    DXGI_FORMAT defaultRenderTargetFormat()
    {
        return renderTargetFormat;
    }

    u32 currentFrameIndex()
    {
        return gfxCommand.getFrameIndex();
    }

    void setDeferredReleasesFlag()
    {
        deferredReleaseFlag[currentFrameIndex()] = 1;
    }

    surface createSurface(platform::window window)
    {
        surfaces.emplace_back(window);
        surfaceId id{ (u32)surfaces.size() - 1 };
        surfaces[id].createSwapChain(dxgiFactory, gfxCommand.getCommandQueue(), renderTargetFormat);

        return surface{ id };
    }

    void removeSurface(surfaceId id)
    {
        gfxCommand.flush();

        //TODO: Properly remove surfaces.
        surfaces[id].~D3D12Surface();
    }

    void resizeSurface(surfaceId id, u32, u32)
    {
        gfxCommand.flush();
        surfaces[id].resize();
    }

    u32 surfaceWidth(surfaceId id)
    {
        return surfaces[id].getWidth();
    }

    u32 surfaceHeight(surfaceId id)
    {
        return surfaces[id].getHeight();
    }

    void renderSurface(surfaceId id)
    {
        /*Wait for the GPU to finish with the command allocator and
        reset the allocator once it is done.
        This frees the memory that was used to store commands.*/
        gfxCommand.beginFrame();
        ID3D12GraphicsCommandList6* commandList{ gfxCommand.getCommandList() };

        const u32 frame_idx{ currentFrameIndex() };
        if (deferredReleaseFlag[frame_idx])
        {
            processDeferredReleases(frame_idx);
        }

        const D3D12Surface& surface{ surfaces[id] };

        surface.present();

        /*Record commands and finish. Once it is done, execute commands,
        signal and increment the fence value for next frame.*/
        gfxCommand.endFrame();
    }
}