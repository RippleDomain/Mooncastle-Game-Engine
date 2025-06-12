#ifndef  EDITOR_INTERFACE
#define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif

#include "CommonHeaders.h"
#include "Id.h"
#include "..\Mooncastle\Components\Entity.h"
#include "..\Mooncastle\Components\Transform.h"

using namespace mooncastle;

namespace 
{
	struct transformComponent
	{
		f32 position[3];  // x, y, z
		f32 rotation[3];  // x, y, z
		f32 scale[3];     // x, y, z

		transform::initInfo toInitInfo()
		{
			using namespace DirectX;

			transform::initInfo info{};

			memcpy(&info.position[0], &position[0], sizeof(f32) * _countof(position));
			memcpy(&info.scale[0], &scale[0], sizeof(f32) * _countof(scale));
			XMFLOAT3A rot{ &rotation[0] };
			XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };

			XMFLOAT4A rot_quat{};
			XMStoreFloat4A(&rot_quat, quat);

			memcpy(&info.rotation[0], &rot_quat.x, sizeof(f32) * _countof(info.rotation));

			return info;
		}
	};

	struct gameEntityDescriptor
	{
		transformComponent transform;
	};

	gameEntity::entity entityFromId(id::idType id)
	{
		return gameEntity::entity{ gameEntity::entityId(id) };
	}
}

EDITOR_INTERFACE
id::idType CreateGameEntity(gameEntityDescriptor* e) 
{
	assert(e);

	gameEntityDescriptor& desc{ *e };

	transform::initInfo transformInfo{ desc.transform.toInitInfo() };
	gameEntity::entityInfo entityInfo{&transformInfo};

	return gameEntity::create(entityInfo).getId();
}

EDITOR_INTERFACE
void RemoveGameEntity(id::idType id) 
{
	assert(id::isValid(id));

	gameEntity::remove(gameEntity::entityId{ id });
}