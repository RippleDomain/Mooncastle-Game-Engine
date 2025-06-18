#pragma once

#include "CommonHeaders.h"

namespace mooncastle::platform
{
	DEFINE_TYPED_ID(windowId);

	class window 
	{
	public:
		constexpr explicit window(windowId id) : id{ id } {}
		constexpr window() : id{ id::invalidId } {}
		constexpr windowId getId() const { return id; }
		constexpr bool isValid() const { return id::isValid(id); }

		void setFullScreen(bool isFullScreen) const;
		bool isFullScreen() const;
		void* handle() const;
		void setCaption(const wchar_t* caption) const;
		math::u32v4 size() const;
		void resize(u32 width, u32 height) const;
		u32 width() const;
		u32 height() const;
		bool isClosed() const;

	private:
		windowId id{ id::invalidId };
	};
}