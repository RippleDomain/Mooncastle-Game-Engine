#include "Common.h"
#include "CommonHeaders.h"
#include "Id.h"
#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"

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

			memcpy(&info.position[0], &position[0], sizeof(position));
			memcpy(&info.scale[0], &scale[0], sizeof(scale));
			XMFLOAT3A rot{ &rotation[0] };
			XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };

			XMFLOAT4A rot_quat{};
			XMStoreFloat4A(&rot_quat, quat);

			memcpy(&info.rotation[0], &rot_quat.x, sizeof(info.rotation));

			return info;
		}
	};

	struct scriptComponent
	{
		script::detail::script_creator scriptCreator;

		script::initInfo toInitInfo()
		{
			script::initInfo info{};
			info.scriptCreator = scriptCreator;
			return info;
		}
	};

	struct gameEntityDescriptor
	{
		transformComponent transform;
		scriptComponent script;
	};

	gameEntity::entity entityFromId(id::idType id)
	{
		return gameEntity::entity{ gameEntity::entityId(id) };
	}
}

EDITOR_INTERFACE id::idType CreateGameEntity(gameEntityDescriptor* e)
{
	assert(e);

	gameEntityDescriptor& desc{ *e };

	transform::initInfo transformInfo{ desc.transform.toInitInfo() };
	script::initInfo scriptInfo{ desc.script.toInitInfo() };
	gameEntity::entityInfo entityInfo
	{ 
		&transformInfo, 
		&scriptInfo 
	};

	return gameEntity::create(entityInfo).getId();
}

EDITOR_INTERFACE void RemoveGameEntity(id::idType id)
{
	assert(id::isValid(id));

	gameEntity::remove(gameEntity::entityId{ id });
}