#include "D3D12Core.h"
#include "D3D12GPass.h"
#include "D3D12Shaders.h"
#include "D3D12Content.h"
#include "D3D12Camera.h"
#include "Shaders/SharedTypes.h"
#include "Components/Transform.h"
#include "Components/Entity.h"

namespace mooncastle::graphics::d3D12::gPass
{
	namespace
	{
		constexpr math::u32v2 initialDimensions{ 100, 100 };

		D3D12RenderTexture    gPassMainBuffer{};
		D3D12DepthBuffer      gPassDepthBuffer{};
		math::u32v2           dimensions{ initialDimensions };

#if _DEBUG
		constexpr f32          clearValue[4]{ 0.5f, 0.5f, 0.5f, 1.f };
#else
		constexpr f32          clearValue[4]{ };
#endif

		struct gPassCache
		{
			utl::vector<id::idType> d3D12RenderItemIDs;

			//When adding new arrays, do not forget to update resize() and structSize.
			id::idType*					entityIDs{ nullptr };
			id::idType*					submeshGPUIDs{ nullptr };
			id::idType*					materialIDs{ nullptr };
			ID3D12PipelineState**		gPassPipelineStates{ nullptr };
			ID3D12PipelineState**		depthPipelineStates{ nullptr };
			ID3D12RootSignature**		rootSignatures{ nullptr };
			materialType::type*			materialTypes{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS*	positionBuffers{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS*	elementBuffers{ nullptr };
			D3D12_INDEX_BUFFER_VIEW*	indexBufferViews{ nullptr };
			D3D12_PRIMITIVE_TOPOLOGY*	primitiveTopologies{ nullptr };
			u32*						elementTypes{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS*	perObjectData{ nullptr };

			constexpr content::renderItem::itemsCache getItemsCache() const
			{
				return 
				{
					entityIDs,
					submeshGPUIDs,
					materialIDs,
					gPassPipelineStates,
					depthPipelineStates
				};
			}

			constexpr content::submesh::viewsCache getViewsCache() const
			{
				return 
				{
					positionBuffers,
					elementBuffers,
					indexBufferViews,
					primitiveTopologies,
					elementTypes
				};
			}

			constexpr content::material::materialsCache getMaterialsCache() const
			{
				return 
				{
					rootSignatures,
					materialTypes
				};
			}

			constexpr u32 getSize() const
			{
				return (u32)d3D12RenderItemIDs.size();
			}

			constexpr void clear()
			{
				d3D12RenderItemIDs.clear();
			}

			constexpr void resize()
			{
				const u64 itemsCount{ d3D12RenderItemIDs.size() };
				const u64 newSize{ itemsCount * structSize };
				const u64 oldSize{ buffer.size() };

				if (newSize > oldSize)
				{
					buffer.resize(newSize);
				}

				if (newSize != oldSize)
				{
					entityIDs = (id::idType*)buffer.data();
					submeshGPUIDs = (id::idType*)(&entityIDs[itemsCount]);
					materialIDs = (id::idType*)(&submeshGPUIDs[itemsCount]);
					gPassPipelineStates = (ID3D12PipelineState**)(&materialIDs[itemsCount]);
					depthPipelineStates = (ID3D12PipelineState**)(&gPassPipelineStates[itemsCount]);
					rootSignatures = (ID3D12RootSignature**)(&depthPipelineStates[itemsCount]);
					materialTypes = (materialType::type*)(&rootSignatures[itemsCount]);
					positionBuffers = (D3D12_GPU_VIRTUAL_ADDRESS*)(&materialTypes[itemsCount]);
					elementBuffers = (D3D12_GPU_VIRTUAL_ADDRESS*)(&positionBuffers[itemsCount]);
					indexBufferViews = (D3D12_INDEX_BUFFER_VIEW*)(&elementBuffers[itemsCount]);
					primitiveTopologies = (D3D12_PRIMITIVE_TOPOLOGY*)(&indexBufferViews[itemsCount]);
					elementTypes = (u32*)(&primitiveTopologies[itemsCount]);
					perObjectData = (D3D12_GPU_VIRTUAL_ADDRESS*)(&elementTypes[itemsCount]);
				}
			}

		private:
			constexpr static u32 structSize
			{
				sizeof(id::idType) +				// entityIDs
				sizeof(id::idType) +				// submeshIDs
				sizeof(id::idType) +				// materialIDs
				sizeof(ID3D12PipelineState*) +		// gPassPipelineStates
				sizeof(ID3D12PipelineState*) +		// depthPipelineStates
				sizeof(ID3D12RootSignature*) +		// rootSignatures
				sizeof(materialType::type) +		// materialTypes
				sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + // positionBuffers
				sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + // elementBuffers
				sizeof(D3D12_INDEX_BUFFER_VIEW) +	// indexBufferViews
				sizeof(D3D12_PRIMITIVE_TOPOLOGY) +	// primitiveTopologies
				sizeof(u32) +						// element_types
				sizeof(D3D12_GPU_VIRTUAL_ADDRESS)	// perObjectData
			};

			utl::vector<u8> buffer;

		} frameCache;

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
				info.clearValue.DepthStencil.Depth = 1.f;
				info.clearValue.DepthStencil.Stencil = 0;

				gPassDepthBuffer = D3D12DepthBuffer{ info };
			}

			NAME_D3D12_OBJECT(gPassMainBuffer.getResource(), L"GPass Main Buffer");
			NAME_D3D12_OBJECT(gPassDepthBuffer.getResource(), L"GPass Depth Buffer");

			return gPassMainBuffer.getResource() && gPassDepthBuffer.getResource();
		}

		void fillPerObjectData(constantBuffer& cBuffer, const D3D12FrameInfo& d3D12Info)
		{
			const gPassCache& cache{ frameCache };
			const u32 renderItemsCount{ (u32)cache.getSize() };
			id::idType currentEntityID{ id::invalidId };
			hlsl::PerObjectData* currentDataPtr{ nullptr };
			constantBuffer& cbuffer{ core::getConstantBuffer() };

			using namespace DirectX;

			for (u32 i{ 0 }; i < renderItemsCount; ++i)
			{
				if (currentEntityID != cache.entityIDs[i])
				{
					currentEntityID = cache.entityIDs[i];
					hlsl::PerObjectData data{};
					transform::getTransformMatrices(gameEntity::entityId{ currentEntityID }, data.World, data.InvWorld);
					XMMATRIX world{ XMLoadFloat4x4(&data.World) };
					XMMATRIX wvp{ XMMatrixMultiply(world, d3D12Info.camera->getViewProjection()) };
					XMStoreFloat4x4(&data.WorldViewProjection, wvp);

					currentDataPtr = cbuffer.allocate<hlsl::PerObjectData>();
					memcpy(currentDataPtr, &data, sizeof(hlsl::PerObjectData));
				}

				assert(currentDataPtr);
				cache.perObjectData[i] = cbuffer.getBufferGPUAddress(currentDataPtr);
			}
		}

		void setRootParams(ID3D12GraphicsCommandList *const commandList, u32 cacheIndex)
		{
			gPassCache& cache{ frameCache };

			assert(cacheIndex < cache.getSize());

			const materialType::type materialType{ cache.materialTypes[cacheIndex] };

			switch (materialType)
			{
			case materialType::opaque:
			{
				using params = opaqueRootParameter;

				commandList->SetGraphicsRootShaderResourceView(params::positionbBuffer, cache.positionBuffers[cacheIndex]);
				commandList->SetGraphicsRootShaderResourceView(params::elementBuffer, cache.elementBuffers[cacheIndex]);
				commandList->SetGraphicsRootConstantBufferView(params::perobjectData, cache.perObjectData[cacheIndex]);
			}
			break;
			}
		}

		void prepareFrame(const D3D12FrameInfo& d3D12Info)
		{
			assert(d3D12Info.info && d3D12Info.camera);
			assert(d3D12Info.info->renderItemIDs && d3D12Info.info->renderItemCount);

			gPassCache& cache{ frameCache };
			cache.clear();

			using namespace content;

			renderItem::getD3D12RenderItemIDs(*d3D12Info.info, cache.d3D12RenderItemIDs);
			cache.resize();
			const u32 itemCount{ cache.getSize() };
			const renderItem::itemsCache itemsCache{ cache.getItemsCache() };
			renderItem::getItems(cache.d3D12RenderItemIDs.data(), itemCount, itemsCache);

			const submesh::viewsCache viewsCache{ cache.getViewsCache() };
			submesh::getViews(itemsCache.submeshGPUIds, itemCount, viewsCache);

			const material::materialsCache materialsCache{ cache.getMaterialsCache() };
			material::getMaterials(itemsCache.materialIDs, itemCount, materialsCache);
		}
	}

	bool initialize()
	{
		return createBuffers(initialDimensions);
	}

	void shutdown()
	{
		gPassMainBuffer.release();
		gPassDepthBuffer.release();
		dimensions = initialDimensions;
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

	void depthPrepass(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& d3D12Info)
	{
		prepareFrame(d3D12Info);

		constantBuffer& cBuffer{ core::getConstantBuffer() };
		fillPerObjectData(cBuffer, d3D12Info);

		const gPassCache& cache{ frameCache };
		const u32 itemsCount{ cache.getSize() };

		ID3D12RootSignature* currentRootSig{ nullptr };
		ID3D12PipelineState* currentPipelineState{ nullptr };

		for (u32 i{ 0 }; i < itemsCount; ++i)
		{
			if (currentRootSig != cache.rootSignatures[i])
			{
				currentRootSig = cache.rootSignatures[i];
				commandList->SetGraphicsRootSignature(currentRootSig);
				commandList->SetGraphicsRootConstantBufferView(opaqueRootParameter::globalShaderData, d3D12Info.globalShaderData);
			}

			if (currentPipelineState != cache.depthPipelineStates[i])
			{
				currentPipelineState = cache.depthPipelineStates[i];
				commandList->SetPipelineState(currentPipelineState);
			}

			setRootParams(commandList, i);

			const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.indexBufferViews[i] };
			const u32 indexCount{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) };

			commandList->IASetIndexBuffer(&ibv);
			commandList->IASetPrimitiveTopology(cache.primitiveTopologies[i]);
			commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
		}
	}

	void render(ID3D12GraphicsCommandList* commandList, const D3D12FrameInfo& d3D12Info)
	{
		const gPassCache& cache{ frameCache };
		const u32 itemsCount{ cache.getSize() };

		ID3D12RootSignature* currentRootSig{ nullptr };
		ID3D12PipelineState* currentPipelineState{ nullptr };

		for (u32 i{ 0 }; i < itemsCount; ++i)
		{
			if (currentRootSig != cache.rootSignatures[i])
			{
				currentRootSig = cache.rootSignatures[i];
				commandList->SetGraphicsRootSignature(currentRootSig);
				commandList->SetGraphicsRootConstantBufferView(opaqueRootParameter::globalShaderData, d3D12Info.globalShaderData);
			}

			if (currentPipelineState != cache.gPassPipelineStates[i])
			{
				currentPipelineState = cache.gPassPipelineStates[i];
				commandList->SetPipelineState(currentPipelineState);
			}

			setRootParams(commandList, i);

			const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.indexBufferViews[i] };
			const u32 indexCount{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) };

			commandList->IASetIndexBuffer(&ibv);
			commandList->IASetPrimitiveTopology(cache.primitiveTopologies[i]);
			commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
		}
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
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
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