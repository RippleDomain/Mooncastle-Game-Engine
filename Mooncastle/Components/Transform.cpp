#include "Transform.h"
#include "Entity.h"

namespace mooncastle::transform
{
	namespace 
	{
		utl::vector<math::v4> rotations;
		utl::vector<math::v3> orientations;
		utl::vector<math::v3> positions;
		utl::vector<math::v3> scales;

		math::v3 calculateOrientation(math::v4 rotation)
		{
			using namespace DirectX;

			XMVECTOR rotationQuaternion{ XMLoadFloat4(&rotation) };
			XMVECTOR front{ XMVectorSet(0.f, 0.f, 1.f, 0.f) };
			math::v3 orientation;

			XMStoreFloat3(&orientation, XMVector3Rotate(front, rotationQuaternion));

			return orientation;
		}
	}

	component create(initInfo info, gameEntity::entity entity) 
	{
		assert(entity.isValid());
		const id::idType entityIndex{ id::index(entity.getId()) };

		if (positions.size() > entityIndex) 
		{
			math::v4 rotation{ info.rotation };
			rotations[entityIndex] = rotation;
			orientations[entityIndex] = calculateOrientation(rotation);
			positions[entityIndex] = math::v3{ info.position };
			scales[entityIndex] = math::v3{ info.scale };
		}
		else
		{
			assert(positions.size() == entityIndex);

			rotations.emplace_back(info.rotation);
			orientations.emplace_back(calculateOrientation(math::v4{ info.rotation }));
			positions.emplace_back(info.position);
			scales.emplace_back(info.scale);
		}
		return component(transformId{ entity.getId() });
	}

	void remove([[maybe_unused]]component c)
	{
		assert(c.isValid());
	}

	math::v4 component::rotation() const 
	{
		assert(isValid());
		return rotations[id::index(id)];
	}

	math::v3 component::orientation() const 
	{
		assert(isValid());
		return orientations[id::index(id)];
	}

	math::v3 component::position() const
	{
		assert(isValid());
		return positions[id::index(id)];
	}

	math::v3 component::scale() const
	{
		assert(isValid());
		return scales[id::index(id)];
	}
}