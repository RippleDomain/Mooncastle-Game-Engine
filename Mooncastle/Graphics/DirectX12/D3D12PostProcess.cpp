#include "D3D12PostProcess.h"
#include "D3D12Shaders.h"
#include "D3D12Surface.h"
#include "D3D12Core.h"
#include "D3D12GPass.h"

namespace mooncastle::graphics::d3D12::ppfx
{
	namespace
	{
		struct fxRootParamIndices
		{
			enum : u32
			{
				rootConstants,
				descriptorTable,
				count
			};
		};

		ID3D12RootSignature*			fxRootSignature{ nullptr };
		ID3D12PipelineState*			fxPSO{ nullptr };

		bool createFXPSOandRootSignature()
		{
			assert(!fxRootSignature);

			d3DX::D3D12DescriptorRange range
			{
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, 0, 0,
				D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
			};

			using idx = fxRootParamIndices;

			d3DX::D3D12RootParameter parameters[2]{};
			parameters[idx::rootConstants].asConstants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
			parameters[idx::descriptorTable].asDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, &range, 1);

			const d3DX::D3D12RootSignatureDescription root_signature{ &parameters[0], _countof(parameters) };
			fxRootSignature = root_signature.create();

			assert(fxRootSignature);

			NAME_D3D12_OBJECT(fxRootSignature, L"Post-Process FX Root Signature");

			struct postProcessStream
			{
				d3DX::D3D12PipelineStateSubobject_rootSignature			rootSignature{ fxRootSignature };
				d3DX::D3D12PipelineStateSubobject_vs					vs{ shaders::getEngineShader(shaders::engineShader::fullscreenTriangleVS) };
				d3DX::D3D12PipelineStateSubobject_ps					ps{ shaders::getEngineShader(shaders::engineShader::postProcessPS) };
				d3DX::D3D12PipelineStateSubobject_primitiveTopology		primitiveTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
				d3DX::D3D12PipelineStateSubobject_renderTargetFormats	renderTargetFormats;
				d3DX::D3D12PipelineStateSubobject_rasterizer			rasterizer{ d3DX::rasterizerState.noCull };
			} stream;

			D3D12_RT_FORMAT_ARRAY rtfArray{};
			rtfArray.NumRenderTargets = 1;
			rtfArray.RTFormats[0] = D3D12Surface::defaultBackBufferFormat;

			stream.renderTargetFormats = rtfArray;

			fxPSO = d3DX::createPipelineState(&stream, sizeof(stream));
			NAME_D3D12_OBJECT(fxPSO, L"Post-Process FX Pipeline State Object");

			return fxRootSignature && fxPSO;
		}
	}

	bool initialize()
	{
		return createFXPSOandRootSignature();
	}

	void shutdown()
	{
		core::release(fxRootSignature);
		core::release(fxPSO);
	}

	void postProcess(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE targetRTV)
	{
		commandList->SetGraphicsRootSignature(fxRootSignature);
		commandList->SetPipelineState(fxPSO);

		using idx = fxRootParamIndices;

		commandList->SetGraphicsRoot32BitConstant(idx::rootConstants, gPass::getMainBuffer().getSRV().index, 0);
		commandList->SetGraphicsRootDescriptorTable(idx::descriptorTable, core::getSRVHeap().getGPUStart());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->OMSetRenderTargets(1, &targetRTV, 1, nullptr);
		commandList->DrawInstanced(3, 1, 0, 0);
	}
}