#pragma once
#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12 {

	class descriptorHeap;

	struct descriptorHandle {
		D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
		u32							index{ u32_invalid_id };

		[[nodiscard]] constexpr bool isValid() const { return cpu.ptr != 0; }
		[[nodiscard]] constexpr bool isShaderVisible() const { return gpu.ptr != 0; }

#ifdef _DEBUG
	private:
		friend class descriptorHeap;
		descriptorHeap* container{ nullptr };
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

		[[nodiscard]] constexpr D3D12_DESCRIPTOR_HEAP_TYPE getType() const { return type; }
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE getCPUStart() const { return cpuStart; }
		[[nodiscard]] constexpr D3D12_GPU_DESCRIPTOR_HANDLE getGPUStart() const { return gpuStart; }
		[[nodiscard]] constexpr ID3D12DescriptorHeap *const getHeap() const { return heap; }
		[[nodiscard]] constexpr u32 getCapacity() const { return capacity; }
		[[nodiscard]] constexpr u32 getSize() const { return size; }
		[[nodiscard]] constexpr u32 getDescriptorSize() const { return descriptorSize; }
		[[nodiscard]] constexpr bool isShaderVisible() const { return gpuStart.ptr != 0; }

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

	struct D3D12BufferInitInfo
	{
		ID3D12Heap1*						heap{ nullptr };
		const void*							data{ nullptr };
		D3D12_RESOURCE_ALLOCATION_INFO1		allocationInfo{};
		D3D12_RESOURCE_STATES				initialState{};
		D3D12_RESOURCE_FLAGS				flags{ D3D12_RESOURCE_FLAG_NONE };
		u32									size{ 0 };
		u32									stride{ 0 };
		u32									elementCount{ 0 };
		u32									alignment{ 0 };
		bool								createUAV{ false };
	};

	class D3D12Buffer
	{
	public:
		D3D12Buffer() = default;
		explicit D3D12Buffer(D3D12BufferInitInfo info, bool isCPUAccessible);

		DISABLE_COPY(D3D12Buffer);

		constexpr D3D12Buffer(D3D12Buffer&& o) : buffer{ o.buffer }, gpuAddress{ o.gpuAddress }, size{ o.size }
		{
			o.reset();
		}

		constexpr D3D12Buffer& operator=(D3D12Buffer&& o)
		{
			assert(this != &o);

			if (this != &o)
			{
				release();
				move(o);
			}

			return *this;
		}

		~D3D12Buffer() { release(); }

		void release();

		[[nodiscard]] constexpr ID3D12Resource* const getBuffer() const { return buffer; }
		[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const { return gpuAddress; }
		[[nodiscard]] constexpr u32 getSize() const { return size; }

	private:
		constexpr void move(D3D12Buffer& o)
		{
			buffer = o.buffer;
			gpuAddress = o.gpuAddress;
			size = o.size;
			o.reset();
		}

		constexpr void reset()
		{
			buffer = nullptr;
			gpuAddress = 0;
			size = 0;
		}

		ID3D12Resource*				buffer{ nullptr };
		D3D12_GPU_VIRTUAL_ADDRESS	gpuAddress{ 0 };
		u32							size{ 0 };
	};

	class constantBuffer
	{
	public:
		constantBuffer() = default;
		explicit constantBuffer(D3D12BufferInitInfo info);

		DISABLE_COPY_AND_MOVE(constantBuffer);

		~constantBuffer() { release(); }

		void release()
		{
			buffer.release();
			cpuAddress = nullptr;
			cpuOffset = 0;
		}

		constexpr void clear() { cpuOffset = 0; }
		[[nodiscard]] u8* const allocate(u32 size);

		template<typename T>
		[[nodiscard]] T* const allocate()
		{
			return(T* const)allocate(sizeof(T));
		}

		[[nodiscard]] constexpr ID3D12Resource* const getBuffer() const { return buffer.getBuffer(); }
		[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress() const { return buffer.getGPUAddress(); }
		[[nodiscard]] constexpr u32 getSize() const { return buffer.getSize(); }
		[[nodiscard]] constexpr u8* const getCPUAddress() const { return cpuAddress; }

		template<typename T>
		[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS getBufferGPUAddress(T* const allocation)
		{
			std::lock_guard lock{ constantBufferMutex };

			assert(cpuAddress);
			if (!cpuAddress) return{};

			const u8* const address{ (const u8* const)allocation };
			assert(address <= cpuAddress + cpuOffset);
			assert(address >= cpuAddress);
			const u64 offset{ (u64)(address - cpuAddress) };

			return buffer.getGPUAddress() + offset;
		}

		[[nodiscard]] constexpr static D3D12BufferInitInfo getDefaultInitInfo(u32 size)
		{
			assert(size);

			D3D12BufferInitInfo info{};
			info.size = size;
			info.alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

			return info;
		}

	private:
		D3D12Buffer		buffer{};
		u8*				cpuAddress{ nullptr };
		u32				cpuOffset{ 0 };
		std::mutex		constantBufferMutex{};
	};

	struct D3D12TextureInitInfo
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
		explicit D3D12Texture(D3D12TextureInitInfo info);

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
		[[nodiscard]] constexpr ID3D12Resource *const getResource() const { return resource; }
		[[nodiscard]] constexpr descriptorHandle getSRV() const { return srv; }

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
		explicit D3D12RenderTexture(D3D12TextureInitInfo info);

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
		explicit D3D12DepthBuffer(D3D12TextureInitInfo info);

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