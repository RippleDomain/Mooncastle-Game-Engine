#pragma once

#include "CommonHeaders.h"

namespace mooncastle::graphics
{
	DEFINE_TYPED_ID(cameraId);

	class camera
	{
	public:
		enum type : u32
		{
			perspective,
			orthographic
		};

		constexpr explicit camera(cameraId id) : id{ id } {}
		constexpr camera() = default;
		constexpr cameraId getId() const { return id; }
		constexpr bool isValid() const { return id::isValid(id); }

		void up(math::v3 up) const;
		void fieldOfView(f32 fov) const;
		void aspectRatio(f32 aspect_ratio) const;
		void viewWidth(f32 width) const;
		void viewHeight(f32 height) const;
		void range(f32 near_z, f32 far_z) const;

		math::m4x4 view() const;
		math::m4x4 projection() const;
		math::m4x4 inverseProjection() const;
		math::m4x4 viewProjection() const;
		math::m4x4 inverseViewProjection() const;
		math::v3 up() const;
		f32 nearZ() const;
		f32 farZ() const;
		f32 fieldOfView() const;
		f32 aspectRatio() const;
		f32 viewWidth() const;
		f32 viewHeight() const;
		type projectionType() const;
		id::idType entityId() const;

	private:
		cameraId id{ id::invalidId };
	};
}