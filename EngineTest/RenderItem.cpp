#include <filesystem>
#include "CommonHeaders.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "Components/Entity.h"
#include "ShaderCompilation.h"
#include "../ContentTools/Geometry.h"

using namespace mooncastle;

gameEntity::entity createOneGameEntity(math::v3 position, math::v3 rotation, const char* scriptName);
void removeGameEntity(gameEntity::entityId);
bool readFile(std::filesystem::path, std::unique_ptr<u8[]>&, u64&);

namespace
{
	id::idType labModelID{ id::invalidId };
	id::idType fanModelID{ id::invalidId };
	id::idType planeModelID{ id::invalidId };

	id::idType labItemID{ id::invalidId };
	id::idType fanItemID{ id::invalidId };
	id::idType planeItemID{ id::invalidId };

	gameEntity::entityId labEntityID{ id::invalidId };
	gameEntity::entityId fanEntityID{ id::invalidId };
	gameEntity::entityId planeEntityID{ id::invalidId };

	id::idType vertexShaderId{ id::invalidId };
	id::idType pixelShaderId{ id::invalidId };
	id::idType materialId{ id::invalidId };

	std::unordered_map<id::idType, gameEntity::entityId> renderItemMap;

	[[nodiscard]] id::idType loadModel(const char* path)
	{
		std::unique_ptr<u8[]> model;
		u64 size{ 0 };
		readFile(path, model, size);

		const id::idType modelID = content::createResource(model.get(), content::assetType::mesh);
		assert(id::isValid(modelID));

		return modelID;
	}

	void loadShaders()
	{
		shaderFileInfo info{};
		info.fileName = "TestShader.hlsl";
		info.function = "TestShaderVS";
		info.type = shaderType::vertex;

		const char* shaderPath{ "..\\..\\EngineTest\\" };

		std::wstring defines[]{ L"ELEMENTS_TYPE=1", L"ELEMENTS_TYPE=3" };
		utl::vector<u32> keys;
		keys.emplace_back(tools::elements::elementTypes::staticNormal);
		keys.emplace_back(tools::elements::elementTypes::staticNormalTexture);

		utl::vector<std::wstring> extraArgs{};
		utl::vector<std::unique_ptr<u8[]>> vertexShaders;
		utl::vector<const u8*> vertexShadersPointers;

		for (u32 i{ 0 }; i < _countof(defines); ++i)
		{
			extraArgs.clear();
			extraArgs.emplace_back(L"-D");
			extraArgs.emplace_back(defines[i]);
			vertexShaders.emplace_back(std::move(compileShader(info, shaderPath, extraArgs)));

			assert(vertexShaders.back().get());
			vertexShadersPointers.emplace_back(vertexShaders.back().get());
		}

		extraArgs.clear();

		info.function = "TestShaderPS";
		info.type = shaderType::pixel;

		auto pixelShader = compileShader(info, shaderPath, extraArgs);

		assert(pixelShader.get());

		vertexShaderId = content::addShaderGroup(vertexShadersPointers.data(), (u32)vertexShadersPointers.size(), keys.data());

		const u8* pixelShaders[]{ pixelShader.get() };
		pixelShaderId = content::addShaderGroup(&pixelShaders[0], 1, &u32_invalid_id);
	}

	void createMaterial()
	{
		assert(id::isValid(vertexShaderId) && id::isValid(pixelShaderId));

		graphics::materialInitInfo info{};

		info.shaderIDs[graphics::shaderType::vertex] = vertexShaderId;
		info.shaderIDs[graphics::shaderType::pixel] = pixelShaderId;
		info.type = graphics::materialType::opaque;

		materialId = content::createResource(&info, content::assetType::material);
	}

	void removeItem(gameEntity::entityId entityID, id::idType itemID, id::idType modelID)
	{
		if (id::isValid(itemID))
		{
			graphics::removeRenderItem(itemID);
			auto pair = renderItemMap.find(itemID);

			if (pair != renderItemMap.end())
			{
				removeGameEntity(pair->second);
			}
		}

		if (id::isValid(modelID))
		{
			content::destroyResource(modelID, content::assetType::mesh);
		}
	}
}

void createRenderItems()
{
	auto first = std::thread{ [] { planeModelID = loadModel("..\\..\\x64\\plane_model.model"); } };
	auto second = std::thread{ [] { labModelID = loadModel("..\\..\\x64\\lab_model.model"); } };
	auto third = std::thread{ [] { fanModelID = loadModel("..\\..\\x64\\fan_model.model"); } };
	auto fourth = std::thread{ [] { loadShaders(); } };

	labEntityID = createOneGameEntity({}, {}, nullptr).getId();
	planeEntityID = createOneGameEntity({ 0.f, 1.3f, -6.6f }, {}, "shipScript").getId();
	fanEntityID = createOneGameEntity({ -10.47f, 5.93f, -6.47f }, {}, "fanScript").getId();

	first.join();
	second.join();
	third.join();
	fourth.join();

	createMaterial();
	id::idType materials[]{ materialId };

	planeItemID = graphics::addRenderItem(planeEntityID, planeModelID, _countof(materials), &materials[0]);
	labItemID = graphics::addRenderItem(labEntityID, labModelID, _countof(materials), &materials[0]);
	fanItemID = graphics::addRenderItem(fanEntityID, fanModelID, _countof(materials), &materials[0]);

	renderItemMap[planeItemID] = planeEntityID;
	renderItemMap[labItemID] = labEntityID;
	renderItemMap[fanItemID] = fanEntityID;
}

void destroyRenderItems()
{
	removeItem(planeEntityID, planeItemID, planeModelID);
	removeItem(labEntityID, labItemID, labModelID);
	removeItem(fanEntityID, fanItemID, fanModelID);

	//Remove material.
	if (id::isValid(materialId))
	{
		content::destroyResource(materialId, content::assetType::material);
	}

	//Removes shaders and textures.
	if (id::isValid(vertexShaderId))
	{
		content::destroyShaderGroup(vertexShaderId);
	}
	if (id::isValid(pixelShaderId))
	{
		content::destroyShaderGroup(pixelShaderId);
	}
}

void getRenderItems(id::idType* items, [[maybe_unused]] u32 count)
{
	assert(count == 3);
	items[0] = planeItemID;
	items[1] = labItemID;
	items[2] = fanItemID;
}