#pragma once

#include "ComponentsCommon.h"

namespace mooncastle::transform
{
	struct initInfo
	{
		f32 position[3]{};              //x, y, z
		f32 rotation[4]{};              //x, y, z, w (quaternion)
		f32 scale[3]{ 1.f, 1.f, 1.f };  //x, y, z
	};

    struct componentFlags 
    {
        enum flags : u32 
        {
            rotation = 0x01,
            orientation = 0x02,
            position = 0x04,
            scale = 0x08,
            all = rotation | orientation | position | scale
        };
    };

    struct componentCache
    {
        math::v4 rotation;
        math::v3 orientation;
        math::v3 position;
        math::v3 scale;
        transformId id;
        u32 flags;
    };

	component create(initInfo info, gameEntity::entity entity);
	void remove(component c);
	void getTransformMatrices(const gameEntity::entityId id, math::m4x4& world, math::m4x4& inverseWorld);
    void getUpdatedComponentFlags(const gameEntity::entityId *const ids, u32 count, u8 *const flags);
    void update(const componentCache *const cache, u32 count);
}