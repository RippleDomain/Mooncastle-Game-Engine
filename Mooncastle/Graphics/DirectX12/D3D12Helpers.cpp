#include "D3D12Helpers.h"
#include "D3D12Core.h"
#include "D3D12Upload.h"

namespace mooncastle::graphics::d3D12::d3DX
{
	namespace
	{

	}

	void transitionResource(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after,
		D3D12_RESOURCE_BARRIER_FLAGS flags /*= D3D12_RESOURCE_BARRIER_FLAG_NONE*/,
		u32 subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
	{
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = flags;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		barrier.Transition.Subresource = subresource;

		commandList->ResourceBarrier(1, &barrier);
	}

	ID3D12RootSignature* createRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& desc)
	{
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDescription{};
		versionedDescription.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedDescription.Desc_1_1 = desc;

		using namespace Microsoft::WRL;

		ComPtr<ID3DBlob> signatureBlob{ nullptr };
		ComPtr<ID3DBlob> errorBlob{ nullptr };

		HRESULT hr{ S_OK };

		if (FAILED(hr = D3D12SerializeVersionedRootSignature(&versionedDescription, &signatureBlob, &errorBlob)))
		{
			DEBUG_OP(const char* errorMsg{ errorBlob ? (const char*)errorBlob->GetBufferPointer() : "" });
			DEBUG_OP(OutputDebugStringA(errorMsg));

			return nullptr;
		}

		assert(signatureBlob);

		ID3D12RootSignature* sigature{ nullptr };

		DXCall(hr = core::device()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),signatureBlob->GetBufferSize(), IID_PPV_ARGS(&sigature)));

		if (FAILED(hr))
		{
			core::release(sigature);
		}

		return sigature;
	}

	ID3D12PipelineState* createPipelineState(D3D12_PIPELINE_STATE_STREAM_DESC desc)
	{
		assert(desc.pPipelineStateSubobjectStream && desc.SizeInBytes);
		ID3D12PipelineState* pso{ nullptr };
		DXCall(core::device()->CreatePipelineState(&desc, IID_PPV_ARGS(&pso)));
		assert(pso);

		return pso;
	}

	ID3D12PipelineState* createPipelineState(void* stream, u64 streamSize)
	{
		assert(stream && streamSize);
		D3D12_PIPELINE_STATE_STREAM_DESC desc{};
		desc.SizeInBytes = streamSize;
		desc.pPipelineStateSubobjectStream = stream;

		return createPipelineState(desc);
	}

	ID3D12Resource* createBuffer(u32 bufferSize, const void* data, bool isCPUAccessible/* = false*/,
			D3D12_RESOURCE_STATES state/* = D3D12_RESOURCE_STATE_COMMON*/,
			D3D12_RESOURCE_FLAGS flags/* = D3D12_RESOURCE_FLAG_NONE*/,
			ID3D12Heap* heap/* = nullptr*/, u64 heapOffset/* = 0*/)
	{
		assert(bufferSize);

		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = bufferSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc = { 1,0 };
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = isCPUAccessible ? D3D12_RESOURCE_FLAG_NONE : flags;

		//The buffer will be only used for upload or as constant buffer.
		assert(desc.Flags == D3D12_RESOURCE_FLAG_NONE || desc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		ID3D12Resource* resource{ nullptr };
		const D3D12_RESOURCE_STATES resourceState{ isCPUAccessible ? D3D12_RESOURCE_STATE_GENERIC_READ : state };

		if (heap)
		{
			DXCall(core::device()->CreatePlacedResource(heap, heapOffset, &desc, resourceState, nullptr, IID_PPV_ARGS(&resource)));
		}
		else
		{
			DXCall(core::device()->CreateCommittedResource(isCPUAccessible ? &heapProperties.uploadHeap : &heapProperties.defaultHeap,
				D3D12_HEAP_FLAG_NONE, &desc, resourceState, nullptr, IID_PPV_ARGS(&resource)));
		}

		if (data)
		{
			/*If we have initial data which we'd like to be able to change later, we set isCPUAccessible
			to true. If we only want to upload data to be used by the GPU, then isCPUAccessible
			should be set to false.*/
			if (isCPUAccessible)
			{
				//range's Begin and End fields are set to 0, to indicate that he CPU is not reading any data.
				const D3D12_RANGE range{};
				void* cpuAddress{ nullptr };
				DXCall(resource->Map(0, &range, reinterpret_cast<void**>(&cpuAddress)));

				assert(cpuAddress);

				memcpy(cpuAddress, data, bufferSize);
				resource->Unmap(0, nullptr);
			}
			else
			{
				upload::D3D12UploadContext context{ bufferSize };
				memcpy(context.getCPUAddress(), data, bufferSize);
				context.getCommandList()->CopyResource(resource, context.getUploadBuffer());
				context.endUpload();
			}
		}

		assert(resource);

		return resource;
	}
}