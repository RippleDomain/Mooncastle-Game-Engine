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
			D3D12_INDEX_BUFFER_VIEW* const		indexElementBuffers;
			D3D12_PRIMITIVE_TOPOLOGY* const		primitiveTopologies;
			u32* const							elementsTypes;
		};

		id::idType add(const u8* &data);
		void remove(id::idType id);
	}

	namespace texture 
	{
		id::idType add(const u8* const);
		void remove(id::idType);
		void getDescriptorIndices(const id::idType* const textureIDs, u32 idCount, u32* const indices);
	}

	namespace material
	{
		id::idType add(materialInitInfo data);
		void remove(id::idType id);
	}
}