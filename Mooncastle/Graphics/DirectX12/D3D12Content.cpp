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
			D3D12_VERTEX_BUFFER_VIEW          positionBufferView{};
			D3D12_VERTEX_BUFFER_VIEW          elementBufferView{};
			D3D12_INDEX_BUFFER_VIEW           indexBufferView{};
			D3D_PRIMITIVE_TOPOLOGY            primitiveTopology;
			u32                               elementType{};
		};

		utl::freeList<ID3D12Resource*>        submeshBuffers{};
		utl::freeList<submeshView>            submeshViews{};
		std::mutex                            submeshMutex{};

		utl::freeList<D3D12Texture>           textures{};
		std::mutex                            textureMutex{};

		utl::vector<ID3D12RootSignature*>     rootSignatures{};
		std::unordered_map<u64, id::idType>   materialMap{}; //Maps a material's type and flags to its ID.
		utl::freeList<std::unique_ptr<u8[]>>  materials{};
		std::mutex                            materialMutex{};

		id::idType createRootSignature(materialType::type type, shaderFlags::flags flags);

		class D3D12MaterialStream
		{
		public:
			DISABLE_COPY_AND_MOVE(D3D12MaterialStream);

			explicit D3D12MaterialStream(u8 *const materialBuffer) : buffer{ materialBuffer }
			{
				initialize();
			}

			explicit D3D12MaterialStream(std::unique_ptr<u8[]>& materialBuffer, materialInitInfo info)
			{
				assert(!materialBuffer);

				u32 shaderCount{ 0 };
				u32 flags{ 0 };

				for (u32 i{ 0 }; i < shaderType::count; ++i)
				{
					if (id::isValid(info.shaderIDs[i]))
					{
						++shaderCount;
						flags |= (1 << i);
					}
				}

				assert(shaderCount && flags);

				const u32 bufferSize
				{
					sizeof(materialType::type) +                           //Material type.
					sizeof(shaderFlags::flags) +                           //Shader flags.
					sizeof(id::idType) +                                   //Root signature ID.
					sizeof(u32) +                                          //Texture count.
					sizeof(id::idType) * shaderCount +                     //Shader IDs.
					(sizeof(id::idType) + sizeof(u32)) * info.textureCount //Texture IDs and descriptor indices (maybe 0 if no textures used).
				};

				materialBuffer = std::make_unique<u8[]>(bufferSize);
				buffer = materialBuffer.get();

				u8 *const newBuffer{ buffer };

				*(materialType::type*)newBuffer = info.type;
				*(shaderFlags::flags*)(&newBuffer[shaderFlagsIndex]) = (shaderFlags::flags)flags;
				*(id::idType*)(&newBuffer[rootSignatureIndex]) = createRootSignature(info.type, (shaderFlags::flags)flags);
				*(u32*)(&newBuffer[textureCountIndex]) = info.textureCount;

				initialize();

				if (info.textureCount)
				{
					memcpy(textureIDs, info.textureIDs, info.textureCount * sizeof(id::idType));
					texture::getDescriptorIndices(textureIDs, info.textureCount, descriptorIndices);
				}

				u32 shaderIndex{ 0 };

				for (u32 i{ 0 }; i < shaderType::count; ++i)
				{
					if (id::isValid(info.shaderIDs[i]))
					{
						shaderIDs[shaderIndex] = info.shaderIDs[i];
						++shaderIndex;
					}
				}

				assert(shaderIndex == (u32)_mm_popcnt_u32(shaderFlags));
			}

			[[nodiscard]] constexpr u32 getTextureCount() const { return textureCount; }
			[[nodiscard]] constexpr materialType::type getMaterialType() const { return type; }
			[[nodiscard]] constexpr shaderFlags::flags getShaderFlags() const { return shaderFlags; }
			[[nodiscard]] constexpr id::idType getRootSigID() const { return rootSignatureID; }
			[[nodiscard]] constexpr id::idType* getTextureIDs() const { return textureIDs; }
			[[nodiscard]] constexpr u32* getDescriptorIndices() const { return descriptorIndices; }
			[[nodiscard]] constexpr id::idType* getShaderIDs() const { return shaderIDs; }

		private:
			void initialize()
			{
				assert(buffer);
				u8 *const newBuffer{ buffer };

				type = *(materialType::type*)newBuffer;
				shaderFlags = *(shaderFlags::flags*)(&newBuffer[shaderFlagsIndex]);
				rootSignatureID = *(id::idType*)(&newBuffer[rootSignatureIndex]);
				textureCount = *(u32*)(&newBuffer[textureCountIndex]);
				shaderIDs = (id::idType*)(&newBuffer[textureCountIndex + sizeof(u32)]);
				textureIDs = textureCount ? &shaderIDs[_mm_popcnt_u32(shaderFlags)] : nullptr;
				descriptorIndices = textureCount ? (u32*)(&textureIDs[textureCount]) : nullptr;
			}

			constexpr static u32		shaderFlagsIndex{ sizeof(materialType::type) };
			constexpr static u32		rootSignatureIndex{ shaderFlagsIndex + sizeof(shaderFlags::flags) };
			constexpr static u32		textureCountIndex{ rootSignatureIndex + sizeof(id::idType) };

			u8*						buffer;
			id::idType*			    textureIDs;
			u32*					descriptorIndices;
			id::idType*			    shaderIDs;
			id::idType				rootSignatureID;
			u32						textureCount;
			materialType::type		type;
			shaderFlags::flags		shaderFlags;
		};

		D3D_PRIMITIVE_TOPOLOGY getD3DPrimitiveTopology(primitiveTopology::type type)
		{
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

		constexpr D3D12_ROOT_SIGNATURE_FLAGS getRootSignatureFlags(shaderFlags::flags flags)
		{
			D3D12_ROOT_SIGNATURE_FLAGS defaultFlags{ d3DX::D3D12RootSignatureDescription::defaultFlags };

			if (flags & shaderFlags::vertex)			defaultFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
			if (flags & shaderFlags::hull)				defaultFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			if (flags & shaderFlags::domain)			defaultFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			if (flags & shaderFlags::geometry)			defaultFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			if (flags & shaderFlags::pixel)				defaultFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			if (flags & shaderFlags::amplification)		defaultFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
			if (flags & shaderFlags::mesh)				defaultFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

			return defaultFlags;
		}

		id::idType createRootSignature(materialType::type type, shaderFlags::flags flags)
		{
			assert(type < materialType::count);
			static_assert(sizeof(type) == sizeof(u32) && sizeof(flags) == sizeof(u32));

			const u64 key{ ((u64)type << 32) | flags };
			auto pair = materialMap.find(key);

			if (pair != materialMap.end())
			{
				assert(pair->first == key);
				return pair->second;
			}

			ID3D12RootSignature* rootSignature{ nullptr };

			switch (type)
			{
			case materialType::opaque:
			{
				using params = gPass::opaqueRootParameter;
				d3DX::D3D12RootParameter parameters[params::count]{};

				D3D12_SHADER_VISIBILITY bufferVisibility{};
				D3D12_SHADER_VISIBILITY dataVisibility{};

				if (flags & shaderFlags::vertex)
				{
					bufferVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
					dataVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
				}
				else if (flags & shaderFlags::mesh)
				{
					bufferVisibility = D3D12_SHADER_VISIBILITY_MESH;
					dataVisibility = D3D12_SHADER_VISIBILITY_MESH;
				}

				if ((flags & shaderFlags::hull) || (flags & shaderFlags::geometry) || (flags & shaderFlags::amplification))
				{
					bufferVisibility = D3D12_SHADER_VISIBILITY_ALL;
					dataVisibility = D3D12_SHADER_VISIBILITY_ALL;
				}

				if ((flags & shaderFlags::pixel) || (flags & shaderFlags::compute))
				{
					dataVisibility = D3D12_SHADER_VISIBILITY_ALL;
				}

				parameters[params::globalShaderData].asCBV(D3D12_SHADER_VISIBILITY_ALL, 0);
				parameters[params::perobjectData].asCBV(dataVisibility, 1);
				parameters[params::positionbBuffer].asSRV(bufferVisibility, 0);
				parameters[params::elementBuffer].asSRV(bufferVisibility, 1);
				parameters[params::srvIndices].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 2);
				parameters[params::directionalLights].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 3);
				parameters[params::cullableLights].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 4);
				parameters[params::lightGrid].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 5);
				parameters[params::lightIndexlist].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 6);

				rootSignature = d3DX::D3D12RootSignatureDescription{ &parameters[0], _countof(parameters), getRootSignatureFlags(flags) }.create();
			}
			break;
			}

			assert(rootSignature);
			const id::idType id{ (id::idType)rootSignatures.size() };
			rootSignatures.emplace_back(rootSignature);
			materialMap[key] = id;
			NAME_D3D12_OBJECT_INDEXED(rootSignature, key, L"GPass Root Signature = key");

			return id;
		}
	}

	bool initialize()
	{
		return true;
	}

	void shutdown()
	{
		for (auto& item : rootSignatures)
		{
			core::release(item);
			materialMap.clear();
			rootSignatures.clear();
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

			view.primitiveTopology = getD3DPrimitiveTopology((primitiveTopology::type)primitiveTopology);
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

	namespace texture
	{
		void getDescriptorIndices(const id::idType* const textureIDs, u32 idCount, u32* const indices)
		{
			assert(textureIDs && idCount && indices);

			std::lock_guard lock{ textureMutex };

			for (u32 i{ 0 }; i < idCount; ++i)
			{
				indices[i] = textures[i].getSRV().index;
			}
		}
	}

	namespace material
	{
		/*The expected output format:
		struct 
		{
			materialType::type	type;
			shaerFlags::flags	flags;
			id::idType			rootSignatureID;
			u32					textureCount;
			id::idType			shader_ids[shaderCount];
			id::idType			textureIDs[textureCount;
			u32*				descriptorIndices[textureCount];
		} D3D12Material*/
		id::idType add(materialInitInfo info)
		{
			std::unique_ptr<u8[]> buffer;
			std::lock_guard lock{ materialMutex };
			D3D12MaterialStream stream{ buffer, info };

			assert(buffer);

			return materials.add(std::move(buffer));
		}

		void remove(id::idType id)
		{
			std::lock_guard lock{ materialMutex };
			materials.remove(id);
		}
	}
}