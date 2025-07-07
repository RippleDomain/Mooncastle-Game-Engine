#include "D3D12Helpers.h"
#include "D3D12Core.h"

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
}