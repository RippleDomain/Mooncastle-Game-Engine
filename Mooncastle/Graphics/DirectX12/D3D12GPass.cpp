#include "D3D12Core.h"
#include "D3D12GPass.h"
#include "D3D12Shaders.h"

namespace mooncastle::graphics::d3D12::gPass
{
	namespace
	{
		struct gPassRootParamIndices 
		{
			enum : u32 
			{
				rootConstants,
				count
			};
		};

		constexpr math::u32v2 initialDimensions{ 100, 100 };

		D3D12RenderTexture    gPassMainBuffer{};
		D3D12DepthBuffer      gPassDepthBuffer{};
		math::u32v2           dimensions{ initialDimensions };

		ID3D12RootSignature*  gPassRootSignature{ nullptr };
		ID3D12PipelineState*  gPassPSO{ nullptr };

#if _DEBUG
		constexpr f32          clearValue[4]{ 0.5f, 0.5f, 0.5f, 1.f };
#else
		constexpr f32          clearValue[4]{ };
#endif

		bool createBuffers(math::u32v2 size)
		{
			assert(size.x && size.y);

			gPassMainBuffer.release();
			gPassDepthBuffer.release();

			D3D12_RESOURCE_DESC desc{};
			desc.Alignment = 0; //0 is the same as 64KB (or 4MB for MSAA).
			desc.DepthOrArraySize = 1;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			desc.Format = mainBufferFormat;
			desc.Height = size.y;
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.MipLevels = 0; //Makes space for all MIP levels.
			desc.SampleDesc = { 1, 0 };
			desc.Width = size.x;

			//Creates the main buffer.
			{
				D3D12TextureInitInfo info{};
				info.desc = &desc;
				info.initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				info.clearValue.Format = desc.Format;
				memcpy(&info.clearValue.Color, &clearValue[0], sizeof(clearValue));
				gPassMainBuffer = D3D12RenderTexture{ info };
			}

			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			desc.Format = depthBufferFormat;
			desc.MipLevels = 1;

			//Creates the depth buffer.
			{
				D3D12TextureInitInfo info{};
				info.desc = &desc;
				info.initialState = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
				info.clearValue.Format = desc.Format;
				info.clearValue.DepthStencil.Depth = 0.f;
				info.clearValue.DepthStencil.Stencil = 0;

				gPassDepthBuffer = D3D12DepthBuffer{ info };
			}

			NAME_D3D12_OBJECT(gPassMainBuffer.getResource(), L"GPass Main Buffer");
			NAME_D3D12_OBJECT(gPassDepthBuffer.getResource(), L"GPass Depth Buffer");

			return gPassMainBuffer.getResource() && gPassDepthBuffer.getResource();
		}

		bool createGPassPSOAndRootSignature()
		{
			assert(!gPassRootSignature && !gPassPSO);

			//Creates GPass root signature.
			using index = gPassRootParamIndices;
			d3DX::D3D12RootParameter parameters[index::count]{};
			parameters[0].asConstants(3, D3D12_SHADER_VISIBILITY_PIXEL, 1);
			d3DX::D3D12RootSignatureDescription rootSignature{ &parameters[0], index::count };
			rootSignature.Flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			gPassRootSignature = rootSignature.create();
			assert(gPassRootSignature);

			NAME_D3D12_OBJECT(gPassRootSignature, L"GPass Root Signature");

			//Creates GPass PSO.
			struct gPassStream 
			{
				d3DX::D3D12PipelineStateSubobject_rootSignature       rootSignature{ gPassRootSignature };
				d3DX::D3D12PipelineStateSubobject_vs                  vs{ shaders::getEngineShader(shaders::engineShader::fullscreenTriangleVS) };
				d3DX::D3D12PipelineStateSubobject_ps                  ps{ shaders::getEngineShader(shaders::engineShader::fillColorPS) };
				d3DX::D3D12PipelineStateSubobject_primitiveTopology   primitiveTopology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
				d3DX::D3D12PipelineStateSubobject_renderTargetFormats renderTargetFormats;
				d3DX::D3D12PipelineStateSubobject_depthStencilFormat  depthStencilFormat{ depthBufferFormat };
				d3DX::D3D12PipelineStateSubobject_rasterizer          rasterizer{ d3DX::rasterizerState.noCull };
				d3DX::D3D12PipelineStateSubobject_depthStencil1       depth{ d3DX::depthState.disabled };
			} stream;

			D3D12_RT_FORMAT_ARRAY rtfArray{};
			rtfArray.NumRenderTargets = 1;
			rtfArray.RTFormats[0] = mainBufferFormat;

			stream.renderTargetFormats = rtfArray;

			gPassPSO = d3DX::createPipelineState(&stream, sizeof(stream));
			NAME_D3D12_OBJECT(gPassPSO, L"GPass Pipeline State Object");

			return gPassRootSignature && gPassPSO;
		}
	}

	bool initialize()
	{
		return createBuffers(initialDimensions) && createGPassPSOAndRootSignature();
	}

	void shutdown()
	{
		gPassMainBuffer.release();
		gPassDepthBuffer.release();
		dimensions = initialDimensions;
		core::release(gPassRootSignature);
		core::release(gPassPSO);
	}

	const D3D12RenderTexture& getMainBuffer()
	{
		return gPassMainBuffer;
	}

	const D3D12DepthBuffer& getDepthBuffer()
	{
		return gPassDepthBuffer;
	}

	void setSize(math::u32v2 size) 
	{
		math::u32v2& d{ dimensions };

		if (size.x > d.x || size.y > d.y)
		{
			d = { std::max(size.x, d.x), std::max(size.y, d.y) };
			createBuffers(d);
		}
	}

	void depthPrepass(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& info)
	{

	}

	void render(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& info)
	{
		commandList->SetGraphicsRootSignature(gPassRootSignature);
		commandList->SetPipelineState(gPassPSO);

		static u32 frame{ 0 };

		struct 
		{
			f32 width;
			f32 height;
			u32 frame;
		} constants{ (f32)info.surfaceWidth, (f32)info.surfaceHeight, ++frame };

		using index = gPassRootParamIndices;

		commandList->SetGraphicsRoot32BitConstants(index::rootConstants, 3, &constants, 0);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	void addTransitionsForDepthPrepass(d3DX::D3D12ResourceBarrier& barriers)
	{
		barriers.add(gPassDepthBuffer.getResource(),
			D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	void addTransitionsForDepthGPass(d3DX::D3D12ResourceBarrier& barriers)
	{
		barriers.add(gPassMainBuffer.getResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		barriers.add(gPassDepthBuffer.getResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	void addTransitionsForPostProcess(d3DX::D3D12ResourceBarrier& barriers)
	{
		barriers.add(gPassMainBuffer.getResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	void setRenderTargetsForDepthPrepass(ID3D12GraphicsCommandList* commandList)
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gPassDepthBuffer.getDSV() };
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.f, 0, 0, nullptr);
		commandList->OMSetRenderTargets(0, nullptr, 0, &dsv);
	}

	void setRenderTargetsForGPass(ID3D12GraphicsCommandList* commandList)
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE rtv{ gPassMainBuffer.getRTV(0) };
		const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gPassDepthBuffer.getDSV() };

		commandList->ClearRenderTargetView(rtv, clearValue, 0, nullptr);
		commandList->OMSetRenderTargets(1, &rtv, 0, &dsv);
	}
}