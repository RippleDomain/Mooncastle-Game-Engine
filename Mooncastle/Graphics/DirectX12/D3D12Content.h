#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::content
{
	bool initialize();
	void shutdown();

	namespace submesh
	{
		struct viewsCache
		{
			D3D12_GPU_VIRTUAL_ADDRESS* const	positionBuffers;
			D3D12_GPU_VIRTUAL_ADDRESS* const	elementBuffers;
			D3D12_INDEX_BUFFER_VIEW* const		indexElementBufferViews;
			D3D12_PRIMITIVE_TOPOLOGY* const		primitiveTopologies;
			u32* const							elementsTypes;
		};

		id::idType add(const u8* &data);
		void remove(id::idType id);
		void getViews(const id::idType* const gpuIDs, u32 idCount, const viewsCache& cache);
	}

	namespace texture 
	{
		id::idType add(const u8* const);
		void remove(id::idType);
		void getDescriptorIndices(const id::idType* const textureIDs, u32 idCount, u32* const indices);
	}

	namespace material
	{
		struct materialsCache
		{
			ID3D12RootSignature** const	 rootSignatures;
			materialType::type* const    materialTypes;
			u32* *const                  descriptorIndices;
			u32*                         textureCount;
		};

		id::idType add(materialInitInfo data);
		void remove(id::idType id);
		void getMaterials(const id::idType* const materialIDs, u32 materialCount, const materialsCache& cache, u32& descriptorIndexCount);
	}

	namespace renderItem
	{
		struct itemsCache
		{
			id::idType* const			 entityIDs;
			id::idType* const			 submeshGPUIds;
			id::idType* const			 materialIDs;
			ID3D12PipelineState* *const  gPassPSOs;
			ID3D12PipelineState* *const  depthPSOs;
		};

		id::idType add(id::idType entityID, id::idType geometryContentID, u32 materialCount, const id::idType* const materialIDs);
		void remove(id::idType id);
		void getD3D12RenderItemIDs(const frameInfo& info, utl::vector<id::idType>& d3d12RenderItemIDs);
		void getItems(const id::idType* const d3d12RenderItemIDs, u32 idCount, const itemsCache& cache);
	}
}