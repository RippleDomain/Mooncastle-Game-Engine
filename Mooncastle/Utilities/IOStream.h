#pragma once

#include "CommonHeaders.h"

namespace mooncastle::utl
{
	//This utility class is intended for local use only (within functions). Do not keep instances around as member variables!
	class blobStreamReader
	{
	public:
		DISABLE_COPY_AND_MOVE(blobStreamReader);

		explicit blobStreamReader(const u8* buffer) : currentBuffer{ buffer }, currentPosition{ buffer }
		{
			assert(buffer);
		}

		//This template function is intended to read primitive types.
		template<typename T>
		[[nodiscard]] T read()
		{
			static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type.");

			T value{ *((T*)currentPosition) };
			currentPosition += sizeof(T);

			return value;
		}

		//Reads "length" bytes into "buffer". The caller is responsible to allocate enough memory in buffer.
		void read(u8* buffer, size_t length)
		{
			memcpy(buffer, currentPosition, length);
			currentPosition += length;
		}

		void skip(size_t offset)
		{
			currentPosition += offset;
		}

		[[nodiscard]] constexpr const u8* const getBufferStart() const { return currentBuffer; }
		[[nodiscard]] constexpr const u8* const getPosition() const { return currentPosition; }
		[[nodiscard]] constexpr size_t getOffset() const { return currentPosition - currentBuffer; }

	private:
		const u8* const	currentBuffer;
		const u8*		currentPosition;
	};

	//This utility class is intended for local use only (within functions). Do not keep instances around as member variables!
	class blobStreamWriter
	{
	public:
		DISABLE_COPY_AND_MOVE(blobStreamWriter);

		explicit blobStreamWriter(u8* buffer, size_t bufferSize) : currentBuffer{ buffer }, currentPosition{ buffer }, currentBufferSize{ bufferSize }
		{
			assert(buffer && bufferSize);
		}

		//This template function is intended to write primitive types.
		template<typename T>
		void write(T value)
		{
			static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type.");
			assert(&currentPosition[sizeof(T)] <= &currentBuffer[currentBufferSize]);

			*((T*)currentPosition) = value;
			currentPosition += sizeof(T);
		}

		//Writes "length" chars into "buffer".
		void write(const char* buffer, size_t length)
		{
			assert(&currentPosition[length] <= &currentBuffer[currentBufferSize]);

			memcpy(currentPosition, buffer, length);
			currentPosition += length;
		}

		//Writes "length" bytes into "buffer".
		void write(const u8* buffer, size_t length)
		{
			assert(&currentPosition[length] <= &currentBuffer[currentBufferSize]);

			memcpy(currentPosition, buffer, length);
			currentPosition += length;
		}

		void skip(size_t offset)
		{
			assert(&currentPosition[offset] <= &currentBuffer[currentBufferSize]);

			currentPosition += offset;
		}

		[[nodiscard]] constexpr const u8* const getBufferStart() const { return currentBuffer; }
		[[nodiscard]] constexpr const u8* const getBufferEnd() const { return &currentBuffer[currentBufferSize]; }
		[[nodiscard]] constexpr const u8* const getPosition() const { return currentPosition; }
		[[nodiscard]] constexpr size_t getOffset() const { return currentPosition - currentBuffer; }

	private:
		u8* const	currentBuffer;
		u8*			currentPosition;
		size_t		currentBufferSize;
	};
}