#include "D3D12Resources.h"
#include "D3D12Core.h"

namespace mooncastle::graphics::d3D12
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DESCRIPTOR HEAP
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool descriptorHeap::initialize(u32 capacity, bool isShaderVisible)
    {
        std::lock_guard lock{ mutex };

        assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
        assert(!(type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));

        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        {
            isShaderVisible = false;
        }

        release();

        ID3D12Device* const device{ core::device() };
        assert(device);

        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = capacity;
        desc.Type = type;
        desc.NodeMask = 0;

        HRESULT hr{ S_OK };
        DXCall(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

        if (FAILED(hr)) return false;

        freeHandles = std::move(std::make_unique<u32[]>(capacity));
        this->capacity = capacity;
        size = 0;

        for (u32 i{ 0 }; i < capacity; ++i) 
        {
            freeHandles[i] = i;
        }

        DEBUG_OP(for (u32 i{ 0 }; i < frameBufferCount; ++i) assert(deferredFreeIndices[i].empty()));

        descriptorSize = device->GetDescriptorHandleIncrementSize(type);
        cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
        gpuStart = isShaderVisible ? heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

        return true;
    }

    void descriptorHeap::release()
    {
        assert(!size);
        core::deferredRelease(heap);
    }

    void descriptorHeap::processDeferredFree(u32 frameIndex)
    {
        std::lock_guard lock{ mutex };
        assert(frameIndex < frameBufferCount);

        utl::vector<u32>& indices{ deferredFreeIndices[frameIndex] };

        if (!indices.empty())
        {
            for (auto index : indices)
            {
                --size;
                freeHandles[size] = index;
            }

            indices.clear();
        }
    }

    descriptorHandle descriptorHeap::allocate()
    {
        std::lock_guard lock{ mutex };

        assert(heap);
        assert(size < capacity);

        const u32 index{ freeHandles[size] };
        const u32 offset{ index * descriptorSize };
        ++size;

        descriptorHandle handle;
        handle.cpu.ptr = cpuStart.ptr + offset;
        if (isShaderVisible())
        {
            handle.gpu.ptr = gpuStart.ptr + offset;
        }

        DEBUG_OP(handle.container = this);
        DEBUG_OP(handle.index = index);

        return handle;
    }

    void descriptorHeap::free(descriptorHandle& handle)
    {
        if (!handle.isValid()) return;

        std::lock_guard lock{ mutex };

        assert(heap && size);
        assert(handle.container == this);
        assert(handle.cpu.ptr >= cpuStart.ptr);
        assert((handle.cpu.ptr - cpuStart.ptr) % descriptorSize == 0);
        assert(handle.index < capacity);

        const u32 index{ (u32)(handle.cpu.ptr - cpuStart.ptr) / descriptorSize };
        assert(handle.index == index);

        const u32 frameIndex{ core::currentFrameIndex() };
        deferredFreeIndices[frameIndex].push_back(index);
        core::setDeferredReleasesFlag();

        handle = {};
    }
}