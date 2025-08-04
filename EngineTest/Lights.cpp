#include "EngineAPI/GameEntity.h"
#include "EngineAPI/Light.h"
#include "EngineAPI/TransformComponent.h"
#include "Graphics/Renderer.h"

#define RANDOM_LIGHTS 1

using namespace mooncastle;

gameEntity::entity createOneGameEntity(math::v3 position, math::v3 rotation, const char* scriptName);
void removeGameEntity(gameEntity::entityId);

namespace
{
	const u64 leftSet{ 0 };
	const u64 rightSet{ 1 };
	constexpr f32 invRandMax{ 1.f / RAND_MAX };

	utl::vector<graphics::light> lights;
	utl::vector<graphics::light> disabledLights;

	constexpr math::v3 rgbToColor(u8 r, u8 g, u8 b) { return { r / 255.f, g / 255.f, b / 255.f }; }

	f32 random(f32 min = 0.f) { return std::max(min, rand() * invRandMax); }

	void createLight(math::v3 position, math::v3 rotation, graphics::light::type type, u64 lightSetKey)
	{
		const char* scriptName{ nullptr }; //{ type == graphics::light::spot ? "rotatorScript" : nullptr };
		gameEntity::entityId entityId{ createOneGameEntity(position, rotation, scriptName).getId() };

		graphics::lightInitInfo info{};
		info.entityID = entityId;
		info.type = type;
		info.lightSetKey = lightSetKey;
		info.intensity = 1.f;

		info.color = { random(0.2f), random(0.2f), random(0.2f) };

#if RANDOM_LIGHTS

		if (type == graphics::light::point)
		{
			info.pointParams.range = random(0.5f) * 2.f;
			info.pointParams.attenuation = { 1, 1, 1 };
		}
		else if (type == graphics::light::spot)
		{
			info.spotParams.range = random(0.5f) * 2.f;
			info.spotParams.umbra = (random(0.5f) - 0.4f) * math::pi;
			info.spotParams.penumbra = info.spotParams.umbra + (0.1f * math::pi);
			info.spotParams.attenuation = { 1, 1, 1 };
		}

#else
		if (type == graphics::light::point)
		{
			info.pointParams.range = 1.f;
			info.pointParams.attenuation = { 1, 1, 1 };
		}
		else if (type == graphics::light::spot)
		{
			info.spotParams.range = 2.f;
			info.spotParams.umbra = 0.1f * math::pi;
			info.spotParams.penumbra = info.spotParams.umbra + (0.1f * math::pi);
			info.spotParams.attenuation = { 1, 1, 1 };
		}

#endif

		graphics::light light{ graphics::createLight(info) };
		assert(light.isValid());
		lights.push_back(light);
	}
}

void generateLights()
{
	graphics::createLightSet(leftSet);
	graphics::createLightSet(rightSet);

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

	//RIGHT SET
	info.entityID = createOneGameEntity({}, { 0, 0, 0 }, nullptr).getId();
	info.lightSetKey = rightSet;
	info.color = rgbToColor(150, 100, 200);
	lights.emplace_back(graphics::createLight(info));

	info.entityID = createOneGameEntity({}, { math::pi * 0.5f, 0, 0 }, nullptr).getId();
	info.color = rgbToColor(17, 27, 48);
	lights.emplace_back(graphics::createLight(info));

	info.entityID = createOneGameEntity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).getId();
	info.color = rgbToColor(163, 47, 30);
	lights.emplace_back(graphics::createLight(info));

#if !RANDOM_LIGHTS

	createLight({ 0, -3, 0 }, {}, graphics::light::point, leftSet);
	createLight({ 0, 0.2, 1.f }, {}, graphics::light::point, leftSet);
	createLight({ 0, 3, 2.5f }, {}, graphics::light::point, leftSet);
	createLight({ 0, 0.1, 7 }, { 0, 3.14f, 0 }, graphics::light::spot, leftSet);

#else

	srand(37);

	constexpr f32 scale1{ 2 };
	constexpr math::v3 scale{ 1.f * scale1, 0.5f * scale1, 1.f * scale1 };
	constexpr i32 dim{ 10 };

	for (i32 x{ -dim }; x < dim; ++x)
	{
		for (i32 y{ 0 }; y < 2 * dim; ++y)
		{
			for (i32 z{ -dim }; z < dim; ++z)
			{
				createLight({ (f32)(x * scale.x), (f32)(y * scale.y), (f32)(z * scale.z) },
					{ random() * 3.14f, random() * 3.14f, random() * 3.14f },
						random() > 0.5f ? graphics::light::spot : graphics::light::point, leftSet);

				createLight({ (f32)(x * scale.x), (f32)(y * scale.y), (f32)(z * scale.z) },
					{ random() * 3.14f, random() * 3.14f, random() * 3.14f },
						random() > 0.5f ? graphics::light::spot : graphics::light::point, rightSet);
			}
		}
	}

#endif
}

void removeLights()
{
	for (auto& light : lights)
	{
		const gameEntity::entityId id{ light.getEntityID() };
		graphics::removeLight(light.getID(), light.getLightSetKey());
		removeGameEntity(id);
	}
	
	for (auto& light : disabledLights)
	{
		const gameEntity::entityId id{ light.getEntityID() };
		graphics::removeLight(light.getID(), light.getLightSetKey());
		removeGameEntity(id);
	}

	lights.clear();
	disabledLights.clear();

	graphics::removeLightSet(leftSet);
	graphics::removeLightSet(rightSet);
}

void testLights(f32 dt)
{
#if 0
	static f32 t{ 0 };
	t += 0.05f;

	for (u32 i{ 0 }; i < (u32)lights.size(); i++)
	{
		f32 sine{ DirectX::XMScalarSin(t + lights[i].getID()) };
		sine *= sine;
		lights[i].setIntensity(2.f * sine);
	}
#else
	u32 count{ (u32)(random(0.1f) * 100) };

	for (u32 i{ 0 }; i < count; ++i)
	{
		if (!lights.size()) break;

		const u32 index{ (u32)(random() * (lights.size() - 1)) };
		graphics::light light{ lights[index] };
		light.setEnabled(false);
		utl::erase_unordered(lights, index);
		disabledLights.emplace_back(light);
	}

	count = (u32)(random(0.1f) * 50);

	for (u32 i{ 0 }; i < count; ++i)
	{
		if (!lights.size()) break;

		const u32 index{ (u32)(random() * (lights.size() - 1)) };
		graphics::light light{ lights[index] };
		const gameEntity::entityId id{ light.getEntityID() };
		graphics::removeLight(light.getID(), light.getLightSetKey());
		removeGameEntity(id);
		utl::erase_unordered(lights, index);
	}

	count = (u32)(random(0.1f) * 50);

	for (u32 i{ 0 }; i < count; ++i)
	{
		if (!disabledLights.size()) break;

		const u32 index{ (u32)(random() * (disabledLights.size() - 1)) };
		graphics::light light{ disabledLights[index] };
		const gameEntity::entityId id{ light.getEntityID() };
		graphics::removeLight(light.getID(), light.getLightSetKey());
		removeGameEntity(id);
		utl::erase_unordered(disabledLights, index);
	}

	count = (u32)(random(0.1f) * 100);

	for (u32 i{ 0 }; i < count; ++i)
	{
		if (!disabledLights.size()) break;

		const u32 index{ (u32)(random() * (disabledLights.size() - 1)) };
		graphics::light light{ disabledLights[index] };
		light.setEnabled(true);
		utl::erase_unordered(disabledLights, index);
		lights.emplace_back(light);
	}

	constexpr f32 scale1{ 1 };
	constexpr math::v3 scale{ 1.f * scale1, 0.5f * scale1, 1.f * scale1 };
	count = (u32)(random(0.1f) * 50);

	for (u32 i{ 0 }; i < count; ++i)
	{
		math::v3 p1{ (random() * 2 - 1.f) * 13.f * scale.x, random() * 2 * 13.f * scale.y, (random() * 2 - 1.f) * 13.f * scale.z };
		math::v3 p2{ (random() * 2 - 1.f) * 13.f * scale.x, random() * 2 * 13.f * scale.y, (random() * 2 - 1.f) * 13.f * scale.z };
		createLight(p1, { random() * 3.14f, random() * 3.14f, random() * 3.14f }, random() > 0.5f ? graphics::light::spot : graphics::light::point, leftSet);
		createLight(p2, { random() * 3.14f, random() * 3.14f, random() * 3.14f }, random() > 0.5f ? graphics::light::spot : graphics::light::point, rightSet);
	}
#endif
}