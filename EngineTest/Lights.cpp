#include "EngineAPI/GameEntity.h"
#include "EngineAPI/Light.h"
#include "EngineAPI/TransformComponent.h"
#include "Graphics/Renderer.h"

using namespace mooncastle;

gameEntity::entity createOneGameEntity(math::v3 position, math::v3 rotation, const char* scriptName);
void removeGameEntity(gameEntity::entityId);

namespace
{
	const u64 leftSet{ 0 };
	const u64 rightSet{ 1 };

	utl::vector<graphics::light> lights;

	constexpr math::v3 rgbToColor(u8 r, u8 g, u8 b) { return { r / 255.f, g / 255.f, b / 255.f }; }
}

void generateLights()
{
	//LEFT SET
	graphics::lightInitInfo info{};

	info.entityID = createOneGameEntity({}, { 0, 0, 0 }, nullptr).getId();
	info.type = graphics::light::directional;
	info.lightSetKey = leftSet;
	info.intensity = 1.f;
	info.color = rgbToColor(174, 174, 174);
	lights.emplace_back(graphics::createLight(info));

	info.entityID = createOneGameEntity({}, { math::pi * 0.5f, 0, 0 }, nullptr).getId();
	info.color = rgbToColor(17, 27, 48);
	lights.emplace_back(graphics::createLight(info));

	info.entityID = createOneGameEntity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).getId();
	info.color = rgbToColor(63, 47, 30);
	lights.emplace_back(graphics::createLight(info));

	// RIGHT_SET
	info.entityID = createOneGameEntity({}, { 0, 0, 0 }, nullptr).getId();
	info.lightSetKey = rightSet;
	info.color = rgbToColor(150, 100, 200);
	lights.emplace_back(graphics::createLight(info));

	info.entityID = createOneGameEntity({}, { math::pi * 0.5f, 0, 0 }, nullptr).getId();
	info.color = rgbToColor(17, 27, 48);
	lights.emplace_back(graphics::createLight(info));

	info.entityID = createOneGameEntity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).getId();
	info.color = rgbToColor(63, 47, 30);
	lights.emplace_back(graphics::createLight(info));
}

void removeLights()
{
	for (auto& light : lights)
	{
		const gameEntity::entityId id{ light.getEntityID() };
		graphics::removeLight(light.getID(), light.getLightSetKey());
		removeGameEntity(id);
	}

	lights.clear();
}