#pragma once

#include "CommonHeaders.h"

namespace mooncastle::graphics
{
	DEFINE_TYPED_ID(lightId);

	class light
	{
	public:
		enum type : u32
		{
			directional,
			point,
			spot,
			count
		};

		constexpr explicit light(lightId id, u64 newKey) : id{ id }, lightSetKey{ newKey } {}
		constexpr light() = default;
		constexpr lightId getID() const { return id; }
		constexpr u64 getLightSetKey() const { return lightSetKey; }
		constexpr bool isValid() const { return id::isValid(id); }

		void setEnabled(bool isEnabled) const;
		void setIntensity(f32 intensity) const;
		void setColor(math::v3 color) const;
		void setAttenuation(math::v3 attenuation) const;
		void setRange(f32 range) const;
		void setConeAngles(f32 umbra, f32 penumbra) const;

		bool isEnabled() const;
		f32 getIntensity() const;
		math::v3 getColor() const;
		math::v3 getAttenuation() const;
		f32 getRange() const;
		f32 getUmbra() const;
		f32 getPenumbra() const;
		type getLightType() const;
		id::idType getEntityID() const;

	private:
		u64     lightSetKey{ 0 };
		lightId id{ id::invalidId };
	};
}