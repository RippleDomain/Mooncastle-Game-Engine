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
				frustumsIn,
				cullingInfo,
				boundingSpheres,
				lightGridOpaque,
				lightIndexListOpaque,
				count
			};
		};

		struct cullingParams
		{
			D3D12Buffer								frustums;
			D3D12Buffer								lightGridAndIndexList;
			structuredBuffer                        lightIndexCounter;
			hlsl::LightCullingDispatchParameters	gridFrustumsDispatchParameters{};
			hlsl::LightCullingDispatchParameters	lightCullingDispatchParameters{};
			u32										frustumCount{ 0 };
			u32										viewWidth{ 0 };
			u32										viewHeight{ 0 };
			f32										cameraFOV{ 0.f };
			D3D12_GPU_VIRTUAL_ADDRESS				lightIndexListOpaqueBuffer{ 0 };
			bool 									hasLights{ true };
		};

		struct lightCuller
		{
			cullingParams							cullers[frameBufferCount]{};
		};

		constexpr u32                               maxLightCountPerTile{ 256 };

		ID3D12RootSignature*						lightCullingRootSignature{ nullptr };
		ID3D12PipelineState*						gridFrustumPSO{ nullptr };
		ID3D12PipelineState*						lightCullingPSO{ nullptr };
		utl::freeList<lightCuller>					lightCullers{};

		bool createRootSignatures()
		{
			assert(!lightCullingRootSignature);

			using param = lightCullingRootParams;
			d3DX::D3D12RootParameter parameters[param::count]{};

			parameters[param::globalShaderData].asCBV(D3D12_SHADER_VISIBILITY_ALL, 0);
			parameters[param::constants].asCBV(D3D12_SHADER_VISIBILITY_ALL, 1);
			parameters[param::frustumsOutOfIndexCounter].asUAV(D3D12_SHADER_VISIBILITY_ALL, 0);
			parameters[param::frustumsIn].asSRV(D3D12_SHADER_VISIBILITY_ALL, 0);
			parameters[param::cullingInfo].asSRV(D3D12_SHADER_VISIBILITY_ALL, 1);
			parameters[param::boundingSpheres].asSRV(D3D12_SHADER_VISIBILITY_ALL, 2);
			parameters[param::lightGridOpaque].asUAV(D3D12_SHADER_VISIBILITY_ALL, 1);
			parameters[param::lightIndexListOpaque].asUAV(D3D12_SHADER_VISIBILITY_ALL, 3);

			lightCullingRootSignature = d3DX::D3D12RootSignatureDescription{ &parameters[0], _countof(parameters) }.create();
			NAME_D3D12_OBJECT(lightCullingRootSignature, L"Light Culling Root Signature");

			return lightCullingRootSignature != nullptr;
		}

		bool createPSOs()
		{
			{
				assert(!gridFrustumPSO);

				struct
				{
					d3DX::D3D12PipelineStateSubobject_rootSignature rootSignature{ lightCullingRootSignature };
					d3DX::D3D12PipelineStateSubobject_cs cs{ shaders::getEngineShader(shaders::engineShader::gridFrustumsCS) };
				} stream;

				gridFrustumPSO = d3DX::createPipelineState(&stream, sizeof(stream));
				NAME_D3D12_OBJECT(gridFrustumPSO, L"Grid Frustums PSO");
			}
			{
				assert(!lightCullingPSO);

				struct
				{
					d3DX::D3D12PipelineStateSubobject_rootSignature rootSignature{ lightCullingRootSignature };
					d3DX::D3D12PipelineStateSubobject_cs cs{ shaders::getEngineShader(shaders::engineShader::lightCullingCS) };
				} stream;

				lightCullingPSO = d3DX::createPipelineState(&stream, sizeof(stream));
				NAME_D3D12_OBJECT(lightCullingPSO, L"Light Culling PSO");
			}

			return gridFrustumPSO != nullptr && lightCullingPSO != nullptr;
		}

		void resizeBuffers(cullingParams& culler)
		{
			const u32 frustumCount{ culler.frustumCount };
			const u32 frustumBufferSize{ sizeof(hlsl::Frustum) * frustumCount };
			const u32 lightGridBufferSize{ (u32)math::alignSizeUp<sizeof(math::v4)>(sizeof(math::u32v2) * frustumCount) };
			const u32 lightIndexBufferSize{ (u32)math::alignSizeUp<sizeof(math::v4)>(sizeof(u32) * maxLightCountPerTile * frustumCount) };
			const u32 lightGridAndIndexListBufferSize{ lightGridBufferSize + lightIndexBufferSize };

			D3D12BufferInitInfo info{};
			info.alignment = sizeof(math::v4);
			info.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			if (frustumBufferSize > culler.frustums.getSize())
			{
				info.size = frustumBufferSize;
				culler.frustums = D3D12Buffer{ info, false };

				NAME_D3D12_OBJECT_INDEXED(culler.frustums.getBuffer(), frustumCount, L"Light Grid Frustums Buffer - Count");
			}

			if (lightGridAndIndexListBufferSize > culler.lightGridAndIndexList.getSize())
			{
				info.size = lightGridAndIndexListBufferSize;
				culler.lightGridAndIndexList = D3D12Buffer{ info, false };

				const D3D12_GPU_VIRTUAL_ADDRESS lightGridOpaqueBuffer{ culler.lightGridAndIndexList.getGPUAddress() };
				culler.lightIndexListOpaqueBuffer = lightGridOpaqueBuffer + lightGridBufferSize;

				NAME_D3D12_OBJECT_INDEXED(culler.lightGridAndIndexList.getBuffer(), lightGridAndIndexListBufferSize,
					L"Light Grid and Index List Buffer - Size");

				if (!culler.lightIndexCounter.getBuffer())
				{
					info = structuredBuffer::getDefaultInitInfo(1);
					culler.lightIndexCounter = structuredBuffer{ info };
					NAME_D3D12_OBJECT_INDEXED(culler.lightIndexCounter.getBuffer(), core::getCurrentFrameIndex(), L"Light Index Counter Buffer");
				}
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

			//Dispatch parameters for grid frustums.
			{
				hlsl::LightCullingDispatchParameters& params{ culler.gridFrustumsDispatchParameters };
				params.NumThreads = tileCount;
				params.NumThreadGroups.x = (u32)math::alignSizeUp<tileSize>(tileCount.x) / tileSize;
				params.NumThreadGroups.y = (u32)math::alignSizeUp<tileSize>(tileCount.y) / tileSize;
			}

			//Dispatch parameters for light culling.
			{
				hlsl::LightCullingDispatchParameters& params{ culler.lightCullingDispatchParameters };
				params.NumThreads.x = tileCount.x * tileSize;
				params.NumThreads.y = tileCount.y * tileSize;
				params.NumThreadGroups = tileCount;
			}

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
		assert(lightCullingRootSignature && gridFrustumPSO && lightCullingPSO);
		core::deferredRelease(lightCullingRootSignature);
		core::deferredRelease(gridFrustumPSO);
		core::deferredRelease(lightCullingPSO);
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

		hlsl::LightCullingDispatchParameters& params{ culler.lightCullingDispatchParameters };

		params.NumLights = light::getCullableLightCount(d3D12Info.info->lightSetKey);
		params.DepthBufferSrvIndex = gPass::getDepthBuffer().getSRV().index;

		/*We update culler.hasLights after this statement, so the light
		culling shader will run once to clear the buffers when there's no lights.*/
		if (!params.NumLights && !culler.hasLights) return;

		culler.hasLights = params.NumLights > 0;

		constantBuffer& cBuffer{ core::getConstantBuffer() };
		hlsl::LightCullingDispatchParameters *const buffer{ cBuffer.allocate<hlsl::LightCullingDispatchParameters>() };
		memcpy(buffer, &params, sizeof(hlsl::LightCullingDispatchParameters));

		//Make light grid and light index buffers writable.
		barriers.add(culler.lightGridAndIndexList.getBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		barriers.apply(commandList);

		const math::u32v4 clearValue{ 0, 0, 0, 0 };
		culler.lightIndexCounter.clearUAV(commandList, &clearValue.x);

		commandList->SetComputeRootSignature(lightCullingRootSignature);
		commandList->SetPipelineState(lightCullingPSO);

		using param = lightCullingRootParams;

		commandList->SetComputeRootConstantBufferView(param::globalShaderData, d3D12Info.globalShaderData);
		commandList->SetComputeRootConstantBufferView(param::constants, cBuffer.getBufferGPUAddress(buffer));
		commandList->SetComputeRootUnorderedAccessView(param::frustumsOutOfIndexCounter, culler.lightIndexCounter.getGPUAddress());
		commandList->SetComputeRootShaderResourceView(param::frustumsIn, culler.frustums.getGPUAddress());
		commandList->SetComputeRootShaderResourceView(param::cullingInfo, light::getCullingInfoBuffer(d3D12Info.frameIndex));
		commandList->SetComputeRootShaderResourceView(param::boundingSpheres, light::getBoundingSpheresBuffer(d3D12Info.frameIndex));
		commandList->SetComputeRootUnorderedAccessView(param::lightGridOpaque, culler.lightGridAndIndexList.getGPUAddress());
		commandList->SetComputeRootUnorderedAccessView(param::lightIndexListOpaque, culler.lightIndexListOpaqueBuffer);

		commandList->Dispatch(params.NumThreadGroups.x, params.NumThreadGroups.y, 1);

		//Make light grid and light index buffers readable.
		barriers.add(culler.lightGridAndIndexList.getBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	D3D12_GPU_VIRTUAL_ADDRESS getFrustums(id::idType lightCullingID, u32 frameIndex)
	{
		assert(frameIndex < frameBufferCount && id::isValid(lightCullingID));
		return lightCullers[lightCullingID].cullers[frameIndex].frustums.getGPUAddress();
	}

	D3D12_GPU_VIRTUAL_ADDRESS getLightGridOpaque(id::idType lightCullingID, u32 frameIndex)
	{
		assert(frameIndex < frameBufferCount && id::isValid(lightCullingID));
		return lightCullers[lightCullingID].cullers[frameIndex].lightGridAndIndexList.getGPUAddress();
	}

	D3D12_GPU_VIRTUAL_ADDRESS getLightIndexListOpaque(id::idType lightCullingID, u32 frameIndex)
	{
		assert(frameIndex < frameBufferCount && id::isValid(lightCullingID));
		return lightCullers[lightCullingID].cullers[frameIndex].lightIndexListOpaqueBuffer;
	}
}