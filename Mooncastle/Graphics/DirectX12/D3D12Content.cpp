#include "D3D12Content.h"
#include "D3D12Core.h"
#include "Utilities/IOStream.h"
#include "D3D12GPass.h"
#include "Content\ContentToEngine.h"

namespace mooncastle::graphics::d3D12::content
{
	namespace
	{
		struct submeshView
		{
			D3D12_VERTEX_BUFFER_VIEW    positionBufferView{};
			D3D12_VERTEX_BUFFER_VIEW    elementBufferView{};
			D3D12_INDEX_BUFFER_VIEW     indexBufferView{};
			D3D_PRIMITIVE_TOPOLOGY      primitiveTopology;
			u32                         elementType{};
		};

		utl::freeList<ID3D12Resource*>  submeshBuffers{};
		utl::freeList<submeshView>      submeshViews{};
		std::mutex                      submeshMutex{};

		D3D_PRIMITIVE_TOPOLOGY getD3DPrimitiveTopology(mooncastle::content::primitiveTopology::type type)
		{
			using namespace mooncastle::content;

			assert(type < primitiveTopology::count);

			switch (type)
			{
			case primitiveTopology::pointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case primitiveTopology::lineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case primitiveTopology::lineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case primitiveTopology::triangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case primitiveTopology::triangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			}

			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	namespace submesh
	{
		/*Expects 'data' to contain (in order):
		u32 elementsize, u32 vertexcount,
		u32 indexcount, u32 elementsType, u32 primitiveTopology
		u8 positions[sizeof(f32) * 3 * vertextCount],		(sizeof(positions) must be a multiple of 4 bytes. Pad if needed.)
		u8 elements[sizeof(elementSize) * vertextCount],	(sizeof(elements) must be a multiple of 4 bytes. Pad if needed.)
		u8 indices[indexSize * indexCount],
		
		Advances the data pointer.
		Position and element buffers should be padded to be a multiple of 4 bytes in length.
		This 4 bytes is defined as D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE.*/
		id::idType add(const u8*& data)
		{
			utl::blobStreamReader blob{ (const u8*)data };

			const u32 elementSize{ blob.read<u32>() };
			const u32 vertexCount{ blob.read<u32>() };
			const u32 indexCount{ blob.read<u32>() };
			const u32 elementsType{ blob.read<u32>() };
			const u32 primitiveTopology{ blob.read<u32>() };
			const u32 indexSize{ (vertexCount < (1 << 16)) ? sizeof(u16) : sizeof(u32) };

			//Element size may be 0,for position-only vertex formats.
			const u32 positionBufferSize{ sizeof(math::v3) * vertexCount };
			const u32 elementBufferSize{ elementSize * vertexCount };
			const u32 indexBufferSize{ indexSize * indexCount };

			constexpr u32 alignment{ D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE };
			const u32 alignedPositionBufferSize{ (u32)math::alignSizeUp<alignment>(positionBufferSize) };
			const u32 alignedElementBufferSize{ (u32)math::alignSizeUp<alignment>(elementBufferSize) };
			const u32 totalBufferSize{ alignedPositionBufferSize + alignedElementBufferSize + indexBufferSize };

			ID3D12Resource* resource{ d3DX::createBuffer(totalBufferSize, blob.getPosition()) };

			blob.skip(totalBufferSize);
			data = blob.getPosition();

			submeshView view{};
			view.positionBufferView.BufferLocation = resource->GetGPUVirtualAddress();
			view.positionBufferView.SizeInBytes = positionBufferSize;
			view.positionBufferView.StrideInBytes = sizeof(math::v3);

			if (elementSize)
			{
				view.elementBufferView.BufferLocation = resource->GetGPUVirtualAddress() + alignedPositionBufferSize;
				view.elementBufferView.SizeInBytes = elementBufferSize;
				view.elementBufferView.StrideInBytes = elementSize;
			}

			view.indexBufferView.BufferLocation = resource->GetGPUVirtualAddress() + alignedPositionBufferSize + alignedElementBufferSize;
			view.indexBufferView.SizeInBytes = indexBufferSize;
			view.indexBufferView.Format = (indexSize == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

			view.primitiveTopology = getD3DPrimitiveTopology((mooncastle::content::primitiveTopology::type)primitiveTopology);
			view.elementType = elementsType;

			std::lock_guard lock{ submeshMutex };
			submeshBuffers.add(resource);

			return submeshViews.add(view);
		}

		void remove(id::idType id)
		{
			std::lock_guard lock{ submeshMutex };
			submeshViews.remove(id);

			core::deferredRelease(submeshBuffers[id]);
			submeshBuffers.remove(id);
		}
	}
}