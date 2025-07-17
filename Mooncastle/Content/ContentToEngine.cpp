#include "ContentToEngine.h"
#include "Utilities\IOStream.h"
#include "Graphics\Renderer.h"

namespace mooncastle::content
{
	namespace
	{
		class geometryHierarchyStream
		{
		public:
			struct lodOffset
			{
				u16 offset;
				u16 count;
			};

			DISABLE_COPY_AND_MOVE(geometryHierarchyStream);

			geometryHierarchyStream(u8* const newBuffer, u32 lods = u32_invalid_id) : buffer{ newBuffer }
			{
				assert(newBuffer && lods);

				if (lods != u32_invalid_id)
				{
					*((u32*)newBuffer) = lods;
				}

				lodCount = *((u32*)newBuffer);
				thresholds = (f32*)(&newBuffer[sizeof(u32)]);
				lodOffsets = (lodOffset*)(&thresholds[lodCount]);
				gpuIDs = (id::idType*)(&lodOffsets[lodCount]);
			}

			void initializeGPUIDs(u32 lod, id::idType*& ids, u32& idCount)
			{
				assert(lod < lodCount);

				ids = &gpuIDs[lodOffsets[lod].offset];
				idCount = lodOffsets[lod].count;
			}

			u32 lodFromThreshold(f32 threshold)
			{
				assert(threshold > 0);

				if (lodCount == 1) return 0;

				for (u32 i{ lodCount - 1 }; i > 0; i--)
				{
					if (thresholds[i] <= threshold) return i;
				}

				assert(false);
				return 0;
			}

			[[nodiscard]] constexpr u32 getLODCount() const { return lodCount; }
			[[nodiscard]] constexpr f32* getThresholds() const { return thresholds; }
			[[nodiscard]] constexpr lodOffset* getLODOffsets() const { return lodOffsets; }
			[[nodiscard]] constexpr id::idType* getGPUIDs() const { return gpuIDs; }

		private:
			u8* const		            buffer;
			f32*			            thresholds;
			lodOffset*		            lodOffsets;
			id::idType*	                gpuIDs;
			u32				            lodCount;
		};

		utl::freeList<u8*>				geometryHierarchies;
		std::mutex                      geometryMutex;

		//Expects the same data as createGeometryResource().
		u32 getGeometryHierarchyBufferSize(const void* const data)
		{
			assert(data);

			utl::blobStreamReader blob{ (const u8*)data };

			const u32 lodCount{ blob.read<u32>() };
			assert(lodCount);

			//Add the sizes of LOD count, thresholds, and LOD offsets to the size of hierarchy.
			u32 size{ sizeof(u32) + (sizeof(f32) + sizeof(geometryHierarchyStream::lodOffset)) * lodCount };

			for (u32 lodIndex{ 0 }; lodIndex < lodCount; ++lodIndex)
			{
				//Skips threshold.
				blob.skip(sizeof(f32));

				//Adds size of GPU IDs (sizeof(id::idType) * submeshCount).
				size += sizeof(id::idType) * blob.read<u32>();

				//Skips submesh data and go to the next LOD.
				blob.skip(blob.read<u32>());
			}

			return size;
		}

		/*Creates a hierarchy stream for a geometry that has multiple LODs and/or multiple submeshes.
		Expects the same data as createGeometryResource()*/
		id::idType createMeshHierarchy(const void* const data)
		{
			assert(data);

			const u32 size{ getGeometryHierarchyBufferSize(data) };
			u8* const hierarchyBuffer{ (u8* const)malloc(size) };

			utl::blobStreamReader blob{ (const u8*)data };
			const u32 lodCount{ blob.read<u32>() };

			assert(lodCount);

			geometryHierarchyStream stream{ hierarchyBuffer, lodCount };
			u16 submeshIndex{ 0 };
			id::idType* const gpuIDs{ stream.getGPUIDs() };

			for (u32 lodIndex{ 0 }; lodIndex < lodCount; ++lodIndex)
			{
				stream.getThresholds()[lodIndex] = blob.read<f32>();
				const u32 id_count{ blob.read<u32>() };

				assert(id_count < (1 << 16));

				stream.getLODOffsets()[lodIndex] = { submeshIndex, (u16)id_count };
				blob.skip(sizeof(u32)); //Skips over the size of submeshes.

				for (u32 idIndex{ 0 }; idIndex < id_count; ++idIndex)
				{
					const u8* at{ blob.getPosition() };
					gpuIDs[submeshIndex++] = graphics::addSubmesh(at);
					blob.skip((u32)(at - blob.getPosition()));

					assert(submeshIndex < (1 << 16));
				}
			}

			assert([&]() 
			{
				f32 previousThreshold{ stream.getThresholds()[0] };

				for (u32 i{ 1 }; i < lodCount; ++i)
				{
					if (stream.getThresholds()[i] <= previousThreshold) return false;

					previousThreshold = stream.getThresholds()[i];
				}

				return true;
			}());

			std::lock_guard lock{ geometryMutex };

			return geometryHierarchies.add(hierarchyBuffer);
		}

		id::idType createGeometryResource(const void* const data)
		{
			return createMeshHierarchy(data);
		}

		void destroyGeometryResource(id::idType id)
		{
			std::lock_guard lock{ geometryMutex };
			u8* const pointer{ geometryHierarchies[id] };
			geometryHierarchyStream stream{ pointer };
			const u32 lodCount{ stream.getLODCount() };
			u32 idIndex{ 0 };

			for (u32 lod{ 0 }; lod < lodCount; ++lod)
			{
				for (u32 i{ 0 }; i < stream.getLODOffsets()[lod].count; ++i)
				{
					graphics::removeSubmesh(stream.getGPUIDs()[idIndex++]);
				}
			}

			free(pointer);

			geometryHierarchies.remove(id);
		}
	}

	id::idType createResource(const void* const data, assetType::type type)
	{
		assert(data);
		id::idType id{ id::invalidId };

		switch (type)
		{
		case assetType::unknown: break;
		case assetType::animation: break;
		case assetType::audio: break;
		case assetType::material: break;
		case assetType::mesh:
			id = createGeometryResource(data);
			break;
		case assetType::skeleton: break;
		case assetType::texture: break;
		}

		assert(id::isValid(id));

		return id;
	}

	void destroyResource(id::idType id, assetType::type type)
	{
		assert(id::isValid(id));

		switch (type)
		{
		case assetType::unknown: break;
		case assetType::animation: break;
		case assetType::audio: break;
		case assetType::material: break;
		case assetType::mesh:
			destroyGeometryResource(id);
			break;
		case assetType::skeleton: break;
		case assetType::texture: break;
		default:
			assert(false);
			break;
		}
	}
}