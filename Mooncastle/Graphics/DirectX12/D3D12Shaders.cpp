#include "D3D12Shaders.h"
#include "../Content/ContentLoader.h"

namespace mooncastle::graphics::d3D12::shaders
{
	namespace
	{
		typedef struct compiledShader
		{
			u64			size;
			const u8*	byte_code;
		} const * compiledShaderPointer;

		//Each element in this array points to an offset within the shaders blob.
		compiledShaderPointer engineShaders[engineShader::count]{};

		/*This is a chunk of memory that contains all compiled engine shaders.
		The blob is an array of shader byte code consisting of a u64 size and
		an array of bytes.*/
		std::unique_ptr<u8[]> shadersBlob{};

		bool loadEngineShaders()
		{
			assert(!shadersBlob);

			u64 size{ 0 };
			bool result{ content::loadEngineShaders(shadersBlob, size) };
			assert(shadersBlob && size);

			u64 offset{ 0 };
			u32 index{ 0 };

			while (offset < size && result)
			{
				assert(index < engineShader::count);
				compiledShaderPointer& shader{ engineShaders[index] };
				assert(!shader);
				result &= index < engineShader::count && !shader;
				if (!result) break;

				shader = reinterpret_cast<const compiledShaderPointer>(&shadersBlob[offset]);
				offset += sizeof(u64) + shader->size;
				++index;
			}

			assert(offset == size && index == engineShader::count);

			return result;
		}
	}

	bool initialize()
	{
		return loadEngineShaders();
	}

	void shutdown()
	{
		for (u32 i{ 0 }; i < engineShader::count; ++i)
		{
			engineShaders[i] = {};
		}

		shadersBlob.reset();
	}

	D3D12_SHADER_BYTECODE getEngineShader(engineShader::id id)
	{
		assert(id < engineShader::count);
		const compiledShaderPointer shader{ engineShaders[id] };
		assert(shader && shader->size);

		return { &shader->byte_code, shader->size };
	}
}