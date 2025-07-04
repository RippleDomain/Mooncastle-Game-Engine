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

	struct d3D12TextureInitInfo
	{
		ID3D12Resource*                   resource{ nullptr };
		ID3D12Heap1*                      heap{ nullptr };
		D3D12_SHADER_RESOURCE_VIEW_DESC*  srvDesc{ nullptr };
		D3D12_RESOURCE_DESC*              desc{ nullptr };
		D3D12_RESOURCE_ALLOCATION_INFO1   allocationInfo{};
		D3D12_RESOURCE_STATES             initialState{};
		D3D12_CLEAR_VALUE                 clearValue{};
	};

	class D3D12Texture
	{
	public:
		constexpr static u32 maxMIPLevel{ 14 }; //Supports up to 16k resolutions.

		D3D12Texture() = default;
		explicit D3D12Texture(d3D12TextureInitInfo info);

		DISABLE_COPY(D3D12Texture);

		constexpr D3D12Texture(D3D12Texture&& o) : resource{ o.resource }, srv{ o.srv }
		{
			o.reset();
		}

		constexpr D3D12Texture& operator=(D3D12Texture&& o)
		{
			assert(this != &o);

			if (this != &o)
			{
				release();
				move(o);
			}

			return *this;
		}

		void release();
		constexpr ID3D12Resource *const getResource() const { return resource; }
		constexpr descriptorHandle getSRV() const { return srv; }

	private:
		constexpr void move(D3D12Texture& o)
		{
			resource = o.resource;
			srv = o.srv;
			o.reset();
		}

		constexpr void reset()
		{
			resource = nullptr;
			srv = {};
		}

		ID3D12Resource* resource{ nullptr };
		descriptorHandle srv;
	};

	class D3D12RenderTexture
	{
	public:
		D3D12RenderTexture() = default;
		explicit D3D12RenderTexture(d3D12TextureInitInfo info);

		DISABLE_COPY(D3D12RenderTexture);

		constexpr D3D12RenderTexture(D3D12RenderTexture&& o) : texture{ std::move(o.texture) }, mipCount{ o.mipCount }
		{
			for (u32 i{ 0 }; i < mipCount; ++i)
			{
				rtv[i] = o.rtv[i];
			}

			o.reset();
		}

		constexpr D3D12RenderTexture& operator=(D3D12RenderTexture&& o)
		{
			assert(this != &o);

			if (this != &o)
			{
				release();
				move(o);
			}

			return *this;
		}

		~D3D12RenderTexture() 
		{ 
			release(); 
		}

		void release();

		[[nodiscard]] constexpr u32 getMIPCount()const { return mipCount; }
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE getRTV(u32 mipIndex)const { assert(mipIndex < mipCount); return rtv[mipIndex].cpu; }
		[[nodiscard]] constexpr descriptorHandle getSRV()const { return texture.getSRV(); }
		[[nodiscard]] constexpr ID3D12Resource *const getResource()const { return texture.getResource(); }

	private:
		constexpr void move(D3D12RenderTexture &o)
		{
			texture = std::move(o.texture);
			mipCount = o.mipCount;

			for (u32 i{ 0 }; i < mipCount; ++i) 
			{
				rtv[i] = o.rtv[i];
			}

			o.reset();
		}
		constexpr void reset()
		{
			for (u32 i{ 0 }; i < mipCount; ++i)
			{
				rtv[i] = {};
			}
			mipCount = 0;
		}

		D3D12Texture			texture{};
		descriptorHandle		rtv[D3D12Texture::maxMIPLevel]{};
		u32						mipCount{ 0 };
	};

	class D3D12DepthBuffer
	{
	public:
		D3D12DepthBuffer() = default;
		explicit D3D12DepthBuffer(d3D12TextureInitInfo info);

		DISABLE_COPY(D3D12DepthBuffer);

		constexpr D3D12DepthBuffer(D3D12DepthBuffer&& o) : texture{ std::move(o.texture) }, dsv{ o.dsv }
		{
			o.dsv = {};
		}

		constexpr D3D12DepthBuffer& operator=(D3D12DepthBuffer&& o)
		{
			assert(this != &o);
			if (this != &o)
			{
				texture = std::move(o.texture);
				dsv = o.dsv;
				o.dsv = {};
			}
			return *this;
		}

		~D3D12DepthBuffer() 
		{
			release(); 
		}

		void release();

		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE getDSV()const { return dsv.cpu; }
		[[nodiscard]] constexpr descriptorHandle getSRV()const { return texture.getSRV(); }
		[[nodiscard]] constexpr ID3D12Resource *const getResource()const { return texture.getResource(); }

	private:
		D3D12Texture			texture{};
		descriptorHandle		dsv{};
	};
}