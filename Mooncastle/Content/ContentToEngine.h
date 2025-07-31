#pragma once

#include "CommonHeaders.h"

namespace mooncastle::content 
{
	struct assetType
	{
		enum type : u32
		{
			unknown = 0,
			animation,
			audio,
			material,
			mesh,
			skeleton,
			texture,
			count
		};
	};

	struct textureFlags 
	{
		enum flags : u32 
		{
			isHDR = 0x01,
			hasAlpha = 0x02,
			isPremultipliedAlpha = 0x04,
			isImportedAsNormalMap = 0x08,
			isCubeMap = 0x10,
			isVolumeMap = 0x20,
		};
	};

	typedef struct compiledShader
	{
		static constexpr u32 hashLength{ 16 };

		constexpr u64 getByteCodeSize() const { return byteCodeSize; }
		constexpr const u8 *const getHash() const { return &hash[0]; }
		constexpr const u8 *const getByteCode() const { return &byteCode; }
		constexpr const u64 getBufferSize() const { return sizeof(byteCodeSize) + hashLength + byteCodeSize; }
		constexpr static u64 getBufferSize(u64 size) { return sizeof(byteCodeSize) + hashLength + size; }

	private:
		u64			byteCodeSize;
		u8          hash[hashLength];
		u8	        byteCode;
	} const * compiledShaderPointer;

	struct lodOffset
	{
		u16 offset;
		u16 count;
	};

	id::idType createResource(const void* const data, assetType::type type);
	void destroyResource(id::idType id, assetType::type type);

	id::idType addShaderGroup(const u8* const *shaders, u32 shaderCount, const u32 *const keys);
	void destroyShaderGroup(id::idType id);
	compiledShaderPointer getShader(id::idType id, u32 shaderKey);

	void getSubmeshGPUIDs(id::idType geometryContentID, u32 idCount, id::idType* const gpuIDs);
	void getLODOffsets(const id::idType* const geometryIDs, const f32* const thresholds, u32 idCount, utl::vector<lodOffset>& offsets);
}