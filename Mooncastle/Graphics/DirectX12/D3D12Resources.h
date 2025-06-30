#pragma once
#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12 {

	class descriptorHeap;

	struct descriptorHandle {
		D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu{};

		constexpr bool isValid() const { return cpu.ptr != 0; }
		constexpr bool isShaderVisible() const { return gpu.ptr != 0; }

#ifdef _DEBUG
	private:
		friend class descriptorHeap;
		descriptorHeap* container{ nullptr };
		u32 index{ u32_invalid_id };
#endif
	};

	class descriptorHeap
	{
	public:
		explicit descriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : type{ type } {}
		DISABLE_COPY_AND_MOVE(descriptorHeap);
		~descriptorHeap() { assert(!heap); }

		bool initialize(u32 capacity, bool isShaderVisible);
		void release();
		void processDeferredFree(u32 frameIndex);

		[[nodiscard]] descriptorHandle allocate();
		void free(descriptorHandle& handle);

		constexpr D3D12_DESCRIPTOR_HEAP_TYPE getType() const { return type; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE getCpuStart() const { return cpuStart; }
		constexpr D3D12_GPU_DESCRIPTOR_HANDLE getGpuStart() const { return gpuStart; }
		constexpr ID3D12DescriptorHeap *const getHeap() const { return heap; }
		constexpr u32 getCapacity() const { return capacity; }
		constexpr u32 getSize() const { return size; }
		constexpr u32 getDescriptorSize() const { return descriptorSize; }
		constexpr bool isShaderVisible() const { return gpuStart.ptr != 0; }

	private:
		ID3D12DescriptorHeap*        heap;
		D3D12_CPU_DESCRIPTOR_HANDLE  cpuStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE  gpuStart{};
		std::unique_ptr<u32[]>       freeHandles{};
		utl::vector<u32>             deferredFreeIndices[frameBufferCount]{};
		std::mutex                   mutex{};
		u32                          capacity{ 0 };
		u32                          size{ 0 };
		u32                          descriptorSize{};
		D3D12_DESCRIPTOR_HEAP_TYPE   type{};
	};
}