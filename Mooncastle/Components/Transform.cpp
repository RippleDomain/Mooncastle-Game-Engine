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
		utl::vector<u8>			changesDuringFrame;
		u8						readWriteFlag;

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

		void setRotation(transformId id, const math::v4& rotationQuaternion)
		{
			const u32 index{ id::index(id) };
			rotations[index] = rotationQuaternion;
			orientations[index] = calculateOrientation(rotationQuaternion);
			hasTransform[index] = 0;
			changesDuringFrame[index] |= componentFlags::rotation;
		}

		void setOrientation(transformId id, const math::v3&)
		{
			
		}

		void setPosition(transformId id, const math::v3& position)
		{
			const u32 index{ id::index(id) };
			positions[index] = position;
			hasTransform[index] = 0;
			changesDuringFrame[index] |= componentFlags::position;
		}

		void setScale(transformId id, const math::v3& scale)
		{
			const u32 index{ id::index(id) };
			scales[index] = scale;
			hasTransform[index] = 0;
			changesDuringFrame[index] |= componentFlags::scale;
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
			changesDuringFrame[entityIndex] = (u8)componentFlags::all;
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
			changesDuringFrame.emplace_back((u8)componentFlags::all);
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

	void getUpdatedComponentFlags(const gameEntity::entityId *const ids, u32 count, u8 *const flags)
	{
		assert(ids && count && flags);

		readWriteFlag = 1;

		for (u32 i{ 0 }; i < count; ++i)
		{
			assert(gameEntity::entity{ ids[i] }.isValid());
			flags[i] = changesDuringFrame[id::index(ids[i])];
		}
	}

	void update(const componentCache *const cache, u32 count)
	{
		assert(cache && count);

		/*Clearing "changesDuringFrame" happens once every frame when there will be no reads and the caches are
		about to be applied by calling this function. The rest of the current frame will only have writes.*/
		if (readWriteFlag)
		{
			memset(changesDuringFrame.data(), 0, changesDuringFrame.size());
			readWriteFlag = 0;
		}

		for (u32 i{ 0 }; i < count; ++i)
		{
			const componentCache& c{ cache[i] };
			assert(component{ c.id }.isValid());

			if (c.flags & componentFlags::rotation)
			{
				setRotation(c.id, c.rotation);
			}

			if (c.flags & componentFlags::orientation)
			{
				setOrientation(c.id, c.orientation);
			}

			if (c.flags & componentFlags::position)
			{
				setPosition(c.id, c.position);
			}

			if (c.flags & componentFlags::scale)
			{
				setScale(c.id, c.scale);
			}
		}
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