#include "D3D12LightCulling.h"
#include "D3D12Camera.h"
#include "D3D12Core.h"
#include "D3D12GPass.h"
#include "D3D12Light.h"
#include "D3D12Shaders.h"
#include "Shaders/SharedTypes.h"

namespace mooncastle::graphics::d3D12::culling
{
	namespace
	{
		struct lightCullingRootParams
		{
			enum parameter : u32
			{
				globalShaderData,
				constants,
				frustumsOutOfIndexCounter,
				count
			};
		};

		struct cullingParams
		{
			D3D12Buffer								frustums;
			hlsl::LightCullingDispatchParameters	gridFrustumsDispatchParameters{};
			u32										frustumCount{ 0 };
			u32										viewWidth{ 0 };
			u32										viewHeight{ 0 };
			f32										cameraFOV{ 0.f };
		};

		struct lightCuller
		{
			cullingParams							cullers[frameBufferCount]{};
		};

		ID3D12RootSignature*						lightCullingRootSignature{ nullptr };
		ID3D12PipelineState*						gridFrustumPSO{ nullptr };
		utl::freeList<lightCuller>					lightCullers{};

		bool createRootSignatures()
		{
			assert(!lightCullingRootSignature);

			using param = lightCullingRootParams;
			d3DX::D3D12RootParameter parameters[param::count]{};

			parameters[param::globalShaderData].asCBV(D3D12_SHADER_VISIBILITY_ALL, 0);
			parameters[param::constants].asCBV(D3D12_SHADER_VISIBILITY_ALL, 1);
			parameters[param::frustumsOutOfIndexCounter].asUAV(D3D12_SHADER_VISIBILITY_ALL, 0);

			lightCullingRootSignature = d3DX::D3D12RootSignatureDescription{ &parameters[0], _countof(parameters) }.create();
			NAME_D3D12_OBJECT(lightCullingRootSignature, L"Light Culling Root Signature");

			return lightCullingRootSignature != nullptr;
		}

		bool createPSOs()
		{
			assert(!gridFrustumPSO);

			struct
			{
				d3DX::D3D12PipelineStateSubobject_rootSignature rootSignature{ lightCullingRootSignature };
				d3DX::D3D12PipelineStateSubobject_cs cs{ shaders::getEngineShader(shaders::engineShader::gridFrustumsCS) };
			} stream;

			gridFrustumPSO = d3DX::createPipelineState(&stream, sizeof(stream));
			NAME_D3D12_OBJECT(gridFrustumPSO, L"Grid Frustums PSO");

			return gridFrustumPSO != nullptr;
		}

		void resizeBuffers(cullingParams& culler)
		{
			const u32 frustumCount{ culler.frustumCount };
			const u32 frustumBufferSize{ sizeof(hlsl::Frustum) * frustumCount };

			D3D12BufferInitInfo info{};
			info.alignment = sizeof(math::v4);
			info.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			if (frustumBufferSize > culler.frustums.getSize())
			{
				info.size = frustumBufferSize;
				culler.frustums = D3D12Buffer{ info, false };

				NAME_D3D12_OBJECT_INDEXED(culler.frustums.getBuffer(), frustumCount, L"Light Grid Frustums Buffer - Count");
			}
		}

		void resize(cullingParams& culler)
		{
			constexpr u32 tileSize{ lightCullingTileSize };
			assert(culler.viewWidth >= tileSize && culler.viewHeight >= tileSize);

			const math::u32v2 tileCount
			{
				(u32)math::alignSizeUp<tileSize>(culler.viewWidth) / tileSize,
				(u32)math::alignSizeUp<tileSize>(culler.viewHeight) / tileSize
			};

			culler.frustumCount = tileCount.x * tileCount.y;

			hlsl::LightCullingDispatchParameters& params{ culler.gridFrustumsDispatchParameters };
			params.NumThreads = tileCount;
			params.NumThreadGroups.x = (u32)math::alignSizeUp<tileSize>(tileCount.x) / tileSize;
			params.NumThreadGroups.y = (u32)math::alignSizeUp<tileSize>(tileCount.y) / tileSize;

			resizeBuffers(culler);
		}

		void calculateGridFrustums(cullingParams& culler, ID3D12GraphicsCommandList *const commandList,
									const D3D12FrameInfo& d3D12Info, d3DX::D3D12ResourceBarrier& barriers)
		{
			constantBuffer& cBuffer{ core::getConstantBuffer() };
			hlsl::LightCullingDispatchParameters *const buffer{ cBuffer.allocate<hlsl::LightCullingDispatchParameters>() };
			const hlsl::LightCullingDispatchParameters& params{ culler.gridFrustumsDispatchParameters };

			memcpy(buffer, &params, sizeof(hlsl::LightCullingDispatchParameters));

			//Makes frustums buffer writable.
			barriers.add(culler.frustums.getBuffer(), 
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			barriers.apply(commandList);

			using param = lightCullingRootParams;

			commandList->SetComputeRootSignature(lightCullingRootSignature);
			commandList->SetPipelineState(gridFrustumPSO);
			commandList->SetComputeRootConstantBufferView(param::globalShaderData, d3D12Info.globalShaderData);
			commandList->SetComputeRootConstantBufferView(param::constants, cBuffer.getBufferGPUAddress(buffer));
			commandList->SetComputeRootUnorderedAccessView(param::frustumsOutOfIndexCounter, culler.frustums.getGPUAddress());
			commandList->Dispatch(params.NumThreadGroups.x, params.NumThreadGroups.y, 1);

			//Makes frustums buffer readable. cullLights() will apply this transition.
			barriers.add(culler.frustums.getBuffer(), 
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		void _declspec(noinline) resizeAndCalculateGridFrustums(cullingParams& culler,
									ID3D12GraphicsCommandList *const commandList,
									const D3D12FrameInfo& d3D12Info,
									d3DX::D3D12ResourceBarrier& barriers)
		{
			culler.cameraFOV = d3D12Info.camera->getFOV();
			culler.viewWidth = d3D12Info.surfaceWidth;
			culler.viewHeight = d3D12Info.surfaceHeight;

			resize(culler);

			calculateGridFrustums(culler, commandList, d3D12Info, barriers);
		}
	}

	bool initialize()
	{
		return createRootSignatures() && createPSOs() && light::initialize();
	}

	void shutdown()
	{
		light::shutdown();
		assert(lightCullingRootSignature && gridFrustumPSO);
		core::deferredRelease(lightCullingRootSignature);
		core::deferredRelease(gridFrustumPSO);
	}

	id::idType addCuller()
	{
		return lightCullers.add();
	}

	void removeCuller(id::idType id)
	{
		assert(id::isValid(id));
		lightCullers.remove(id);
	}

	void cullLights(ID3D12GraphicsCommandList* const commandList, const D3D12FrameInfo& d3D12Info, d3DX::D3D12ResourceBarrier& barriers)
	{
		const id::idType id{ d3D12Info.lightCullingID };

		assert(id::isValid(id));

		cullingParams& culler{ lightCullers[id].cullers[d3D12Info.frameIndex] };

		if (d3D12Info.surfaceWidth != culler.viewWidth || d3D12Info.surfaceHeight != culler.viewHeight
			|| !math::isEqual(d3D12Info.camera->getFOV(), culler.cameraFOV))
		{
			resizeAndCalculateGridFrustums(culler, commandList, d3D12Info, barriers);
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS getFrustums(id::idType lightCullingID, u32 frameIndex)
	{
		assert(frameIndex < frameBufferCount && id::isValid(lightCullingID));
		return lightCullers[lightCullingID].cullers[frameIndex].frustums.getGPUAddress();
	}

	/*D3D12_GPU_VIRTUAL_ADDRESS getLightGridOpaque(id::idType lightCullingID, u32 frameIndex)
	{
		assert(frameIndex < frameBufferCount && id::isValid(lightCullingID));
		return lightCullers[lightCullingID].cullers[frameIndex].lightGridAndIndexList.getGPUAddress();
	}

	D3D12_GPU_VIRTUAL_ADDRESS getLightIndexListOpaque(id::idType lightCullingID, u32 frameIndex)
	{
		assert(frameIndex < frameBufferCount && id::isValid(lightCullingID));
		return lightCullers[lightCullingID].cullers[frameIndex].lightIndexListOpaqueBuffer;
	}*/
}