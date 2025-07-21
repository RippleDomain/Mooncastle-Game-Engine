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

	typedef struct compiledShader
	{
		static constexpr u32 hashLength{ 16 };

		constexpr u64 getByteCodeSize() const { return byteCodeSize; }
		constexpr const u8 *const getHash() const { return &hash[0]; }
		constexpr const u8 *const getByteCode() const { return &byteCode; }

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

	id::idType createShader(const u8* shaderData);
	void destroyShader(id::idType id);
	compiledShaderPointer getShader(id::idType id);

	void getSubmeshGPUIDs(id::idType geometryContentID, u32 idCount, id::idType* const gpuIDs);
	void getLODOffsets(const id::idType* const geometryIDs, const f32* const thresholds, u32 idCount, utl::vector<lodOffset>& offsets);
}