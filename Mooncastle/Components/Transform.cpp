#include "Transform.h"
#include "Entity.h"

namespace mooncastle::transform
{
	namespace 
	{
		utl::vector<math::m4x4> toWorld;
		utl::vector<math::m4x4> invWorld;
		utl::vector<math::v4>	rotations;
		utl::vector<math::v3>	orientations;
		utl::vector<math::v3>	positions;
		utl::vector<math::v3>	scales;
		utl::vector<u8>			hasTransform;

		void calculateTransformMatrices(id::idType index)
		{
			assert(rotations.size() >= index);
			assert(positions.size() >= index);
			assert(scales.size() >= index);

			using namespace DirectX;
			XMVECTOR r{ XMLoadFloat4(&rotations[index]) };
			XMVECTOR t{ XMLoadFloat3(&positions[index]) };
			XMVECTOR s{ XMLoadFloat3(&scales[index]) };

			XMMATRIX world{ XMMatrixAffineTransformation(s, XMQuaternionIdentity(), r, t) };
			XMStoreFloat4x4(&toWorld[index], world);

			world.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
			XMMATRIX inverseWorld{ XMMatrixInverse(nullptr, world) };
			XMStoreFloat4x4(&invWorld[index], inverseWorld);

			hasTransform[index] = 1;
		}

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
			hasTransform[entityIndex] = 0;
		}
		else
		{
			assert(positions.size() == entityIndex);

			toWorld.emplace_back();
			invWorld.emplace_back();
			rotations.emplace_back(info.rotation);
			orientations.emplace_back(calculateOrientation(math::v4{ info.rotation }));
			positions.emplace_back(info.position);
			scales.emplace_back(info.scale);
			hasTransform.emplace_back((u8)0);
		}
		return component(transformId{ entity.getId() });
	}

	void remove([[maybe_unused]]component c)
	{
		assert(c.isValid());
	}

	void getTransformMatrices(const gameEntity::entityId id, math::m4x4& world, math::m4x4& inverseWorld)
	{
		assert(gameEntity::entity{ id }.isValid());

		const id::idType entityIndex{ id::index(id) };

		if (!hasTransform[entityIndex])
		{
			calculateTransformMatrices(entityIndex);
		}

		world = toWorld[entityIndex];
		inverseWorld = invWorld[entityIndex];
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