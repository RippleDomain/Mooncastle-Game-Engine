#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::upload
{
	class D3D12UploadContext
	{
	public:
		D3D12UploadContext(u32 alignedSize);
		DISABLE_COPY_AND_MOVE(D3D12UploadContext);
		~D3D12UploadContext() { assert(frameIndex == u32_invalid_id); }

		void endUpload();

		[[nodiscard]] constexpr ID3D12GraphicsCommandList* const getCommandList() const { return cmdList; }
		[[nodiscard]] constexpr ID3D12Resource* const getUploadBuffer() const { return uploadBuffer; }
		[[nodiscard]] constexpr void* const getCPUAddress() const { return cpuAddress; }

	private:
		DEBUG_OP(D3D12UploadContext() = default);
		
		ID3D12GraphicsCommandList*	    cmdList{ nullptr };
		ID3D12Resource*					uploadBuffer{ nullptr };
		void*							cpuAddress{ nullptr };
		u32								frameIndex{ u32_invalid_id };
	};

	bool initialize();
	void shutdown();
}