#include "D3D12Content.h"
#include "D3D12Core.h"
#include "Utilities/IOStream.h"
#include "D3D12GPass.h"
#include "D3D12Upload.h"
#include "Content\ContentToEngine.h"

namespace mooncastle::graphics::d3D12::content
{
	namespace
	{
		struct psoId
		{
			id::idType gPassPSOID{ id::invalidId };
			id::idType depthPSOID{ id::invalidId };
		};

		struct submeshView
		{
			D3D12_VERTEX_BUFFER_VIEW					 positionBufferView{};
			D3D12_VERTEX_BUFFER_VIEW					 elementBufferView{};
			D3D12_INDEX_BUFFER_VIEW						 indexBufferView{};
			D3D_PRIMITIVE_TOPOLOGY						 primitiveTopology;
			u32											 elementType{};
		};		

		struct D3D12RenderItem
		{
			id::idType entityID;
			id::idType submeshGPUID;
			id::idType materialID;
			id::idType psoID;
			id::idType depthPSOID;
		};
														 
		utl::freeList<ID3D12Resource*>					 submeshBuffers{};
		utl::freeList<submeshView>						 submeshViews{};
		std::mutex										 submeshMutex{};
														 
		utl::freeList<D3D12Texture>						 textures{};
		utl::freeList<u32>							     descriptorIndices;
		std::mutex										 textureMutex{};
														 
		utl::vector<ID3D12RootSignature*>				 rootSignatures{};
		std::unordered_map<u64, id::idType>				 materialMap{}; //Maps a material's type and flags to its ID.
		utl::freeList<std::unique_ptr<u8[]>>			 materials{};
		std::mutex										 materialMutex{};
														 
		utl::freeList<D3D12RenderItem>					 renderItems;
		utl::freeList<std::unique_ptr<id::idType[]>>	 renderItemIDs;
		std::mutex										 renderItemMutex{};
		utl::vector<ID3D12PipelineState*>				 pipelineStates;
		std::unordered_map<u64, id::idType>				 psoMap;
		std::mutex										 psoMutex{};

		struct
		{
			utl::vector<mooncastle::content::lodOffset>	 lodOffsets;
			utl::vector<id::idType>					     geometryIDs;
		} frameCache;

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

		constexpr D3D_PRIMITIVE_TOPOLOGY getD3DPrimitiveTopology(primitiveTopology::type type)
		{
			assert(type < primitiveTopology::count);

			switch (type)
			{
			case primitiveTopology::pointList:			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case primitiveTopology::lineList:			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case primitiveTopology::lineStrip:			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case primitiveTopology::triangleList:		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case primitiveTopology::triangleStrip:		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			}

			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

		constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE getD3DPrimitiveTopologyType(D3D_PRIMITIVE_TOPOLOGY topology)
		{
			switch (topology)
			{
			case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
			case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
			case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			}

			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
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
				parameters[params::perObjectData].asCBV(dataVisibility, 1);
				parameters[params::positionBuffer].asSRV(bufferVisibility, 0);
				parameters[params::elementBuffer].asSRV(bufferVisibility, 1);
				parameters[params::srvIndices].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 2);
				parameters[params::directionalLights].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 3);
				parameters[params::cullableLights].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 4);
				parameters[params::lightGrid].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 5);
				parameters[params::lightIndexList].asSRV(D3D12_SHADER_VISIBILITY_PIXEL, 6);

				const D3D12_STATIC_SAMPLER_DESC samplers[]
				{
					d3DX::staticSampler(d3DX::samplerState.staticPoint, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
					d3DX::staticSampler(d3DX::samplerState.staticLinear, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL),
					d3DX::staticSampler(d3DX::samplerState.staticAnisotropic, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL),
				};

				rootSignature = d3DX::D3D12RootSignatureDescription
				{
					&parameters[0], _countof(parameters), getRootSignatureFlags(flags),
					&samplers[0], _countof(samplers)
				}.create();
			}
			break;
			}

			assert(rootSignature);
			const id::idType id{ (id::idType)rootSignatures.size() };
			rootSignatures.emplace_back(rootSignature);
			materialMap[key] = id;
			NAME_D3D12_OBJECT_INDEXED(rootSignature, key, L"GPass Root Signature = Key");

			return id;
		}

		id::idType createPSOIfNecessary(const u8* const streamPointer, u64 alignedStreamSize, [[maybe_unused]] bool isDepth)
		{
			const u64 key{ math::calc_crc32_u64(streamPointer, alignedStreamSize) };

			//Checks if PSO already exists.
			{ 
				std::lock_guard lock{ psoMutex };
				auto pair = psoMap.find(key);

				if (pair != psoMap.end())
				{
					assert(pair->first == key);
					return pair->second;
				}
			}

			//Creates a new PSO.
			d3DX::D3D12PipelineStateSubobjectStream* const stream{ (d3DX::D3D12PipelineStateSubobjectStream* const)streamPointer };
			ID3D12PipelineState* pso{ d3DX::createPipelineState(stream, sizeof(d3DX::D3D12PipelineStateSubobjectStream)) };

			//Adds the new PSO's pointer and ID.
			{
				std::lock_guard lock{ psoMutex };
				const id::idType id{ (u32)pipelineStates.size() };

				pipelineStates.emplace_back(pso);

				NAME_D3D12_OBJECT_INDEXED(pipelineStates.back(), key, isDepth ?
					L"Depth-only Pipeline State Object - Key" :
					L"GPass Pipline State Object - Key");
				psoMap[key] = id;

				return id;
			}
		}

#pragma intrinsic(_BitScanForward)
		shaderType::type getShaderType(u32 flag)
		{
			assert(flag);
			unsigned long index;
			_BitScanForward(&index, flag);
			return (shaderType::type)index;
		}

		psoId createPSO(id::idType materialID, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology, u32 elementType)
		{
			constexpr u64 alignedStreamSize{ math::alignSizeUp<sizeof(u64)>(sizeof(d3DX::D3D12PipelineStateSubobjectStream)) };
			u8* const streamPointer{ (u8* const)alloca(alignedStreamSize) };
			ZeroMemory(streamPointer, alignedStreamSize);

			new (streamPointer) d3DX::D3D12PipelineStateSubobjectStream{};

			d3DX::D3D12PipelineStateSubobjectStream& stream{ *(d3DX::D3D12PipelineStateSubobjectStream* const)streamPointer };

			//Locks materials.
			{
				std::lock_guard lock{ materialMutex };
				const D3D12MaterialStream material{ materials[materialID].get() };

				D3D12_RT_FORMAT_ARRAY renderTargetArray{};
				renderTargetArray.NumRenderTargets = 1;
				renderTargetArray.RTFormats[0] = gPass::mainBufferFormat;

				stream.renderTargetFormats = renderTargetArray;
				stream.rootSignature = rootSignatures[material.getRootSigID()];
				stream.primitiveTopology = getD3DPrimitiveTopologyType(primitiveTopology);
				stream.depthStencilFormat = gPass::depthBufferFormat;
				stream.rasterizer = d3DX::rasterizerState.backfaceCull;
				stream.depthStencil1 = d3DX::depthState.reversedReadonly;
				stream.blend = d3DX::blendState.disabled;

				const shaderFlags::flags flags{ material.getShaderFlags() };
				D3D12_SHADER_BYTECODE shaders[shaderType::count]{};
				u32 shaderIndex{ 0 };

				for (u32 i{ 0 }; i < shaderType::count; ++i)
				{
					if (flags & (1 << i))
					{
						const u32 key{ getShaderType(flags & (1 << i)) == shaderType::vertex ? elementType : u32_invalid_id };
						mooncastle::content::compiledShaderPointer shader{ mooncastle::content::getShader(material.getShaderIDs()[shaderIndex], key) };
						assert(shader);

						shaders[i].pShaderBytecode = shader->getByteCode();
						shaders[i].BytecodeLength = shader->getByteCodeSize();
						++shaderIndex;
					}
				}

				stream.vs = shaders[shaderType::vertex];
				stream.ps = shaders[shaderType::pixel];
				stream.ds = shaders[shaderType::domain];
				stream.hs = shaders[shaderType::hull];
				stream.gs = shaders[shaderType::geometry];
				stream.cs = shaders[shaderType::compute];
				stream.as = shaders[shaderType::amplification];
				stream.ms = shaders[shaderType::mesh];
			}

			psoId idPair{};
			idPair.gPassPSOID = createPSOIfNecessary(streamPointer, alignedStreamSize, false);

			stream.ps = D3D12_SHADER_BYTECODE{};
			stream.depthStencil1 = d3DX::depthState.reversed;
			idPair.depthPSOID = createPSOIfNecessary(streamPointer, alignedStreamSize, true);

			return idPair;
		}

		D3D12Texture createResourceFromTextureData(const u8 *const data)
		{
			assert(data);

			utl::blobStreamReader blob{ data };
			const u32 width{ blob.read<u32>() };
			const u32 height{ blob.read<u32>() };
			u32 depth{ 1 };
			u32 arraySize{ blob.read<u32>() };
			const u32 flags{ blob.read<u32>() };
			const u32 mipLevels{ blob.read<u32>() };
			const DXGI_FORMAT format{ (DXGI_FORMAT)blob.read<u32>() };
			const bool is3D{ (flags & mooncastle::content::textureFlags::isVolumeMap) != 0 };

			assert(mipLevels <= D3D12Texture::maxMIPLevel);

			u32 depthPerMIPLevel[D3D12Texture::maxMIPLevel]{};

			for (u32 i{ 0 }; i < D3D12Texture::maxMIPLevel; ++i)
			{
				depthPerMIPLevel[i] = 1;
			}

			if (is3D)
			{
				depth = arraySize;
				arraySize = 1;
				u32 depthPerMIP{ depth };

				for (u32 i{ 0 }; i < mipLevels; ++i)
				{
					depthPerMIPLevel[i] = depthPerMIP;
					depthPerMIP = std::max(depthPerMIP >> 1, (u32)1);
				}
			}

			utl::vector<D3D12_SUBRESOURCE_DATA> subresources{};

			for (u32 i{ 0 }; i < arraySize; ++i)
			{
				for (u32 j{ 0 }; j < mipLevels; ++j)
				{
					const u32 rowPitch{ blob.read<u32>() };
					const u32 slicePitch{ blob.read<u32>() };

					subresources.emplace_back(D3D12_SUBRESOURCE_DATA
					{
						blob.getPosition(),
						rowPitch,
						slicePitch
					});

					//Skips the rest of the slices.
					blob.skip(slicePitch * depthPerMIPLevel[j]);
				}
			}

			D3D12_RESOURCE_DESC desc{};

			desc.Dimension = is3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Alignment = 0;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = is3D ? (u16)depth : (u16)arraySize;
			desc.MipLevels = (u16)mipLevels;
			desc.Format = format;
			desc.SampleDesc = { 1,0 };
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;

			assert(!(flags & mooncastle::content::textureFlags::isCubeMap && (arraySize % 6)));
			const u32 subresourceCount{ arraySize * mipLevels };
			assert(subresourceCount);

			const u32 footprintDataSize{ (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(u32) + sizeof(u64)) * subresourceCount };
			std::unique_ptr<u8[]> footprintData{ std::make_unique<u8[]>(footprintDataSize) };

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT *const layouts{ (D3D12_PLACED_SUBRESOURCE_FOOTPRINT *const)footprintData.get() };
			u32 *const rowCount{ (u32 *const)&layouts[subresourceCount] };
			u64 *const rowSizes{ (u64 *const)&rowCount[subresourceCount] };
			u64 sizeRequired{ 0 };

			ID3D12Device* device{ core::device() };

			device->GetCopyableFootprints(&desc, 0, subresourceCount, 0, layouts, rowCount, rowSizes, &sizeRequired);

			assert(sizeRequired);
			upload::D3D12UploadContext context{ (u32)sizeRequired };
			u8 *const cpuAddress{ (u8 *const)context.getCPUAddress() };

			for (u32 subresourceIndex{ 0 }; subresourceIndex < subresourceCount; ++subresourceIndex)
			{
				const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout{ layouts[subresourceIndex] };
				const u32 subresourceHeight{ rowCount[subresourceIndex] };
				const u32 subresourceDepth{ layout.Footprint.Depth };
				const D3D12_SUBRESOURCE_DATA& subresource{ subresources[subresourceIndex] };

				const D3D12_MEMCPY_DEST copyDestination
				{
					cpuAddress + layout.Offset,
					layout.Footprint.RowPitch,
					layout.Footprint.RowPitch * subresourceHeight
				};

				for (u32 depthIndex{ 0 }; depthIndex < subresourceDepth; ++depthIndex)
				{
					u8 *const sourceSlice{ (u8 *const)subresource.pData + subresource.SlicePitch * depthIndex };
					u8 *const destinationSlice{ (u8 *const)copyDestination.pData + copyDestination.SlicePitch * depthIndex };

					for (u32 rowIndex{ 0 }; rowIndex < subresourceHeight; ++rowIndex)
					{
						memcpy(destinationSlice + copyDestination.RowPitch * rowIndex, sourceSlice + subresource.RowPitch * rowIndex, rowSizes[subresourceIndex]);
					}
				}
			}

			ID3D12Resource* resource{ nullptr };
			DXCall(device->CreateCommittedResource(&d3DX::heapProperties.defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
				D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)));

			ID3D12Resource* uploadBuffer{ context.getUploadBuffer() };
			for (u32 i{ 0 }; i < subresourceCount; ++i)
			{
				D3D12_TEXTURE_COPY_LOCATION source{};
				source.pResource = uploadBuffer;
				source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				source.PlacedFootprint = layouts[i];

				D3D12_TEXTURE_COPY_LOCATION destination{};
				destination.pResource = resource;
				destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				destination.SubresourceIndex = i;

				context.getCommandList()->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
			}

			context.endUpload();

			assert(resource);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription{};
			D3D12TextureInitInfo info{};
			info.resource = resource;

			if (flags & mooncastle::content::textureFlags::isCubeMap)
			{
				assert(arraySize % 6 == 0);

				srvDescription.Format = format;
				srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				if (arraySize > 6)
				{
					srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					srvDescription.TextureCubeArray.MostDetailedMip = 0;
					srvDescription.TextureCubeArray.MipLevels = mipLevels;
					srvDescription.TextureCubeArray.NumCubes = arraySize / 6;
				}
				else
				{
					srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					srvDescription.TextureCube.MostDetailedMip = 0;
					srvDescription.TextureCube.MipLevels = mipLevels;
					srvDescription.TextureCube.ResourceMinLODClamp = 0.0f;
				}

				info.srvDesc = &srvDescription;
			}

			return D3D12Texture{ info };
		}
	}

	bool initialize()
	{
		return true;
	}

	/*We only release data that were created as a side-effect to adding resources here,
	which the user of this module has no control over. The user is obligated to release
	the rest of the data by calling their respective "remove" functions before to shutting down the renderer.
	That way we can make sure the book-keeping of content is correct.*/
	void shutdown()
	{
		for (auto& item : rootSignatures)
		{
			core::release(item);
		}

		materialMap.clear();
		rootSignatures.clear();

		for (auto& item : pipelineStates)
		{
			core::release(item);
		}

		psoMap.clear();
		pipelineStates.clear();
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

		void getViews(const id::idType* const gpuIDs, u32 idCount, const viewsCache& cache)
		{
			assert(gpuIDs && idCount);
			assert(cache.positionBuffers && 
					cache.elementBuffers && 
					cache.indexElementBufferViews &&
					cache.primitiveTopologies && 
					cache.elementsTypes);

			std::lock_guard lock{ submeshMutex };

			for (u32 i{ 0 }; i < idCount; ++i)
			{
				const submeshView& view{ submeshViews[gpuIDs[i]] };
				cache.positionBuffers[i] = view.positionBufferView.BufferLocation;
				cache.elementBuffers[i] = view.elementBufferView.BufferLocation;
				cache.indexElementBufferViews[i] = view.indexBufferView;
				cache.primitiveTopologies[i] = view.primitiveTopology;
				cache.elementsTypes[i] = view.elementType;
			}
		}
	}

	namespace texture
	{
		/*Expects data to contain :
		struct
		{
		    u32 width, height, arraySize (or depth), flags, mipLevels, format,
		 
		    struct 
			  {
		        u32 rowPitch, slicePitch,
		        u8 image[mipLevel][slicePitch * depthPerMIP],
		    } images[]
		} texture*/
		id::idType add(const u8* const data)
		{
			assert(data);
			D3D12Texture texture{ createResourceFromTextureData(data) };

			std::lock_guard lock{ textureMutex };
			const id::idType id{ textures.add(std::move(texture)) };
			descriptorIndices.add(textures[id].getSRV().index);

			return id;
		}

		void remove(id::idType id)
		{
			std::lock_guard lock{ textureMutex };
			textures.remove(id);
			descriptorIndices.remove(id);
		}

		void getDescriptorIndices(const id::idType* const textureIDs, u32 idCount, u32* const indices)
		{
			assert(textureIDs && idCount && indices);

			std::lock_guard lock{ textureMutex };

			for (u32 i{ 0 }; i < idCount; ++i)
			{
				indices[i] = descriptorIndices[textureIDs[i]];
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
			id::idType			shaderIDs[shaderCount];
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

		void getMaterials(const id::idType* const materialIDs, u32 materialCount, const materialsCache& cache, u32& descriptorIndexCount)
		{
			assert(materialIDs && materialCount);
			assert(cache.rootSignatures && cache.materialTypes);

			std::lock_guard lock{ materialMutex };

			u32 totalIndexCount{ 0 };

			for (u32 i{ 0 }; i < materialCount; ++i)
			{
				const D3D12MaterialStream stream{ materials[materialIDs[i]].get() };
				cache.rootSignatures[i] = rootSignatures[stream.getRootSigID()];
				cache.materialTypes[i] = stream.getMaterialType();
				cache.descriptorIndices[i] = stream.getDescriptorIndices();
				cache.textureCount[i] = stream.getTextureCount();
				totalIndexCount += stream.getTextureCount();
			}

			descriptorIndexCount = totalIndexCount;
		}
	}

	namespace renderItem
	{
		/*Creates a buffer that's basically an array of id::idTypes.
		buffer[0] = geometryContentID.
		buffer[1 ... n] = d3d12RenderItemIDs (n is the number of low-level render items which also must be equal the number of submeshes/material IDs).
		buffer[n + 1] = id::invalidIDd (marks the end of d3d12RenderItemIDs array).*/
		id::idType add(id::idType entityID, id::idType geometryContentID, u32 materialCount, const id::idType* const materialIDs)
		{
			assert(id::isValid(entityID) && id::isValid(geometryContentID));
			assert(materialCount && materialIDs);

			id::idType* const gpuIDs{ (id::idType* const)alloca(materialCount * sizeof(id::idType)) };
			mooncastle::content::getSubmeshGPUIDs(geometryContentID, materialCount, gpuIDs);

			submesh::viewsCache viewsCache
			{
				(D3D12_GPU_VIRTUAL_ADDRESS* const)alloca(materialCount * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
				(D3D12_GPU_VIRTUAL_ADDRESS* const)alloca(materialCount * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
				(D3D12_INDEX_BUFFER_VIEW* const)alloca(materialCount * sizeof(D3D12_INDEX_BUFFER_VIEW)),
				(D3D12_PRIMITIVE_TOPOLOGY* const)alloca(materialCount * sizeof(D3D12_PRIMITIVE_TOPOLOGY)),
				(u32* const)alloca(materialCount * sizeof(u32)),
			};

			submesh::getViews(gpuIDs, materialCount, viewsCache);

			//Tthe list of IDs starts with a geometry ID and ends with an invalid ID to mark the beginning and end of the list.
			std::unique_ptr<id::idType[]> items{ std::make_unique<id::idType[]>(sizeof(id::idType) * (1 + (u64)materialCount + 1)) };
			items[0] = geometryContentID;
			id::idType* const itemIDs{ &items[1] };

			D3D12RenderItem *const d3D12Items{ (D3D12RenderItem *const)alloca(materialCount * sizeof(D3D12RenderItem)) };

			for (u32 i{ 0 }; i < materialCount; ++i)
			{
				D3D12RenderItem& item{ d3D12Items[i] };
				item.entityID = entityID;
				item.submeshGPUID = gpuIDs[i];
				item.materialID = materialIDs[i];

				psoId idPair{ createPSO(item.materialID, viewsCache.primitiveTopologies[i], viewsCache.elementsTypes[i])};
				item.psoID = idPair.gPassPSOID;
				item.depthPSOID = idPair.depthPSOID;

				assert(id::isValid(item.submeshGPUID) && id::isValid(item.materialID));
			}

			std::lock_guard lock{ renderItemMutex };

			for (u32 i{ 0 }; i < materialCount; ++i)
			{
				itemIDs[i] = renderItems.add(d3D12Items[i]);
			}

			//Marks the end of the ID list.
			itemIDs[materialCount] = id::invalidId;

			return renderItemIDs.add(std::move(items));
		}

		void remove(id::idType id)
		{
			std::lock_guard lock{ renderItemMutex };
			const id::idType* const itemIDs{ &renderItemIDs[id][1] };

			//The last element in the list of IDs is always an invalid ID.
			for (u32 i{ 0 }; itemIDs[i] != id::invalidId; ++i)
			{
				renderItems.remove(itemIDs[i]);
			}

			renderItemIDs.remove(id);
		}

		//This will be called at least once per frame, so it must run fast. Therefore it uses the predefined frameCache structure.
		void getD3D12RenderItemIDs(const frameInfo& info, utl::vector<id::idType>& d3d12RenderItemIDs)
		{
			assert(info.renderItemIDs && info.thresholds && info.renderItemCount);
			assert(d3d12RenderItemIDs.empty());

			frameCache.lodOffsets.clear();
			frameCache.geometryIDs.clear();

			const u32 count{ info.renderItemCount };
			std::lock_guard lock{ renderItemMutex };

			for (u32 i{ 0 }; i < count; ++i)
			{
				const id::idType* const buffer{ renderItemIDs[info.renderItemIDs[i]].get() };
				frameCache.geometryIDs.emplace_back(buffer[0]);
			}

			mooncastle::content::getLODOffsets(frameCache.geometryIDs.data(), info.thresholds, count, frameCache.lodOffsets);

			assert(frameCache.lodOffsets.size() == count);

			u32 d3d12RenderItemCount{ 0 };

			for (u32 i{ 0 }; i < count; ++i)
			{
				d3d12RenderItemCount += frameCache.lodOffsets[i].count;
			}

			assert(d3d12RenderItemCount);

			//This is grow only, because resize() will only resize the vector if it is too small.
			d3d12RenderItemIDs.resize(d3d12RenderItemCount);

			u32 itemIndex{ 0 };

			for (u32 i{ 0 }; i < count; ++i)
			{
				const id::idType* const item_ids{ &renderItemIDs[info.renderItemIDs[i]][1] };
				const mooncastle::content::lodOffset& lodOffset{ frameCache.lodOffsets[i] };
				memcpy(&d3d12RenderItemIDs[itemIndex], &item_ids[lodOffset.offset], sizeof(id::idType) * lodOffset.count);
				itemIndex += lodOffset.count;

				assert(itemIndex <= d3d12RenderItemCount);
			}

			assert(itemIndex == d3d12RenderItemCount);
		}

		void getItems(const id::idType* const d3d12RenderItemIDs, u32 idCount, const itemsCache& cache)
		{
			assert(d3d12RenderItemIDs && idCount);
			assert(cache.entityIDs && cache.submeshGPUIds && cache.materialIDs && cache.gPassPSOs && cache.depthPSOs);

			std::lock_guard lock1{ renderItemMutex };
			std::lock_guard lock2{ psoMutex };

			for (u32 i{ 0 }; i < idCount; ++i)
			{
				const D3D12RenderItem& item{ renderItems[d3d12RenderItemIDs[i]] };
				cache.entityIDs[i] = item.entityID;
				cache.submeshGPUIds[i] = item.submeshGPUID;
				cache.materialIDs[i] = item.materialID;
				cache.gPassPSOs[i] = pipelineStates[item.psoID];
				cache.depthPSOs[i] = pipelineStates[item.depthPSOID];
			}
		}
	}
}