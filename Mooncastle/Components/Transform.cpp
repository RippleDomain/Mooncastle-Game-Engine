#include "Transform.h"
#include "Entity.h"

namespace mooncastle::transform
{
	namespace 
	{
		utl::vector<math::v3> positions;
		utl::vector<math::v4> rotations;
		utl::vector<math::v3> scales;
	}

	component create(initInfo info, gameEntity::entity entity) 
	{
		assert(entity.isValid());
		const id::idType entityIndex{ id::index(entity.getId()) };

		if (positions.size() > entityIndex) 
		{
			rotations[entityIndex] = math::v4(info.rotation);
			positions[entityIndex] = math::v3(info.position);
			scales[entityIndex] = math::v3(info.scale);
		}
		else
		{
			assert(positions.size() == entityIndex);

			rotations.emplace_back(info.rotation);
			positions.emplace_back(info.position);
			scales.emplace_back(info.scale);
		}

		return component(transformId{ (id::idType)positions.size() - 1 });
	}

	void remove(component c)
	{
		assert(c.isValid());
	}

	math::v4 component::rotation() const 
	{
		assert(isValid());
		return rotations[id::index(_id)];
	}

	math::v3 component::position() const
	{
		assert(isValid());
		return positions[id::index(_id)];
	}

	math::v3 component::scale() const
	{
		assert(isValid());
		return scales[id::index(_id)];
	}
}