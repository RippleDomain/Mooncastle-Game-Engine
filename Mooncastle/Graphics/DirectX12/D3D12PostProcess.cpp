#include "D3D12PostProcess.h"
#include "D3D12Shaders.h"
#include "D3D12Surface.h"
#include "D3D12Core.h"
#include "D3D12GPass.h"
#include "D3D12LightCulling.h"

namespace mooncastle::graphics::d3D12::ppfx
{
	namespace
	{
		struct fxRootParamIndices
		{
			enum : u32
			{
				globalShaderData,
				rootConstants,
				frustums,
				lightGridOpaque,
				count
			};
		};

		ID3D12RootSignature*			fxRootSignature{ nullptr };
		ID3D12PipelineState*			fxPSO{ nullptr };

		bool createFXPSOandRootSignature()
		{
			assert(!fxRootSignature);

			using idx = fxRootParamIndices;

			d3DX::D3D12RootParameter parameters[idx::count]{};
			parameters[idx::globalShaderData].asCBV(D3D12_SHADER_VISIBILITY_PIXEL, 0);
			parameters[idx::rootConstants].asConstants(2, D3D12_SHADER_VISIBILITY_PIXEL, 1);
			parameters[idx::frustums].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 0);
			parameters[idx::lightGridOpaque].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 1);

			const D3D12_STATIC_SAMPLER_DESC samplers[]
			{
				d3DX::staticSampler(d3DX::samplerState.staticPoint, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
				d3DX::staticSampler(d3DX::samplerState.staticLinear, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL),
			};

			d3DX::D3D12RootSignatureDescription rootSignature
			{
				&parameters[0], 
				_countof(parameters), 

				d3DX::D3D12RootSignatureDescription::defaultFlags,

				&samplers[0],
				_countof(samplers)
			};

			rootSignature.Flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			fxRootSignature = rootSignature.create();

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

	void postProcess(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& d3D12Info, D3D12_CPU_DESCRIPTOR_HANDLE targetRTV)
	{
		const u32 frameIndex{ d3D12Info.frameIndex };
		const id::idType lightCullingID{ d3D12Info.lightCullingID };

		commandList->SetGraphicsRootSignature(fxRootSignature);
		commandList->SetPipelineState(fxPSO);

		using idx = fxRootParamIndices;

		commandList->SetGraphicsRootConstantBufferView(idx::globalShaderData, d3D12Info.globalShaderData);
		commandList->SetGraphicsRoot32BitConstant(idx::rootConstants, gPass::getMainBuffer().getSRV().index, 0);
		commandList->SetGraphicsRoot32BitConstant(idx::rootConstants, gPass::getDepthBuffer().getSRV().index, 1);
		commandList->SetGraphicsRootShaderResourceView(idx::frustums, culling::getFrustums(lightCullingID, frameIndex));
		commandList->SetGraphicsRootShaderResourceView(idx::lightGridOpaque, culling::getLightGridOpaque(lightCullingID, frameIndex));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->OMSetRenderTargets(1, &targetRTV, 1, nullptr);
		commandList->DrawInstanced(3, 1, 0, 0);
	}
}