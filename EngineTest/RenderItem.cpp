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
	id::idType excaliburModelID{ id::invalidId };

	id::idType labItemID{ id::invalidId };
	id::idType fanItemID{ id::invalidId };
	id::idType planeItemID{ id::invalidId };
	id::idType excaliburItemID{ id::invalidId };

	gameEntity::entityId labEntityID{ id::invalidId };
	gameEntity::entityId fanEntityID{ id::invalidId };
	gameEntity::entityId planeEntityID{ id::invalidId };
	gameEntity::entityId excaliburEntityID{ id::invalidId };

	struct textureUsage
	{
		enum usage : u32 
		{
			ambientOcclusion = 0,
			baseColor,
			emissive,
			metal,
			roughness,
			normal,
			count
		};
	};

	id::idType textureIDs[textureUsage::count];

	id::idType vertexShaderID{ id::invalidId };
	id::idType pixelShaderID{ id::invalidId };
	id::idType texturedPixelShaderID{ id::invalidId };

	id::idType defaultMaterialID{ id::invalidId };
	id::idType excaliburMaterialID{ id::invalidId };

	std::unordered_map<id::idType, gameEntity::entityId> renderItemMap;

	//Loading the test model.
	[[nodiscard]] id::idType loadModel(const char* path)
	{
		std::unique_ptr<u8[]> model;
		u64 size{ 0 };
		readFile(path, model, size);

		const id::idType modelID = content::createResource(model.get(), content::assetType::mesh);
		assert(id::isValid(modelID));

		return modelID;
	}

	//Loading the test texture.
	[[nodiscard]] id::idType loadTexture(const char* path)
	{
		std::unique_ptr<u8[]> texture;
		u64 size{ 0 };
		readFile(path, texture, size);

		const id::idType textureID = content::createResource(texture.get(), content::assetType::texture);
		assert(id::isValid(textureID));

		return textureID;
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

		utl::vector<std::unique_ptr<u8[]>> pixelShaders;

		pixelShaders.emplace_back(compileShader(info, shaderPath, extraArgs));
		assert(pixelShaders.back().get());

		defines[0] = L"TEXTURED_MTL=1";
		extraArgs.emplace_back(L"-D");
		extraArgs.emplace_back(defines[0]);

		pixelShaders.emplace_back(compileShader(info, shaderPath, extraArgs));
		assert(pixelShaders.back().get());

		vertexShaderID = content::addShaderGroup(vertexShadersPointers.data(), (u32)vertexShadersPointers.size(), keys.data());

		const u8* pixelShaderPtrs[]{ pixelShaders[0].get() };
		pixelShaderID = content::addShaderGroup(pixelShaderPtrs, 1, &u32_invalid_id);

		pixelShaderPtrs[0] = pixelShaders[1].get();
		texturedPixelShaderID = content::addShaderGroup(pixelShaderPtrs, 1, &u32_invalid_id);
	}

	void createMaterial()
	{
		assert(id::isValid(vertexShaderID) && id::isValid(pixelShaderID));

		graphics::materialInitInfo info{};

		info.shaderIDs[graphics::shaderType::vertex] = vertexShaderID;
		info.shaderIDs[graphics::shaderType::pixel] = pixelShaderID;
		info.type = graphics::materialType::opaque;

		defaultMaterialID = content::createResource(&info, content::assetType::material);

		info.shaderIDs[graphics::shaderType::pixel] = texturedPixelShaderID;
		info.textureCount = textureUsage::count;
		info.textureIDs = &textureIDs[0];

		excaliburMaterialID = content::createResource(&info, content::assetType::material);
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
	assert(std::filesystem::exists("..\\..\\x64\\labModel.model"));
	assert(std::filesystem::exists("..\\..\\x64\\fanModel.model"));
	assert(std::filesystem::exists("..\\..\\x64\\planeModel.model"));
	assert(std::filesystem::exists("..\\..\\x64\\excaliburModel.model"));

	memset(&textureIDs[0], 0xff, sizeof(id::idType) * _countof(textureIDs));

	std::thread threads[]
	{
		std::thread{ [] { textureIDs[textureUsage::ambientOcclusion] = loadTexture("..\\..\\x64\\excaliburAO.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::baseColor] = loadTexture("..\\..\\x64\\excaliburBaseColor.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::emissive] = loadTexture("..\\..\\x64\\excaliburEmissive.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::metal] = loadTexture("..\\..\\x64\\excaliburMetal.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::roughness] = loadTexture("..\\..\\x64\\excaliburRoughness.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::normal] = loadTexture("..\\..\\x64\\excaliburNormal.texture"); }},

		std::thread{ [] { labModelID = loadModel("..\\..\\x64\\labModel.model"); } },
		std::thread{ [] { fanModelID = loadModel("..\\..\\x64\\fanModel.model"); } },
		std::thread{ [] { planeModelID = loadModel("..\\..\\x64\\planeModel.model"); } },
		std::thread{ [] { excaliburModelID = loadModel("..\\..\\x64\\excaliburModel.model"); } },
		std::thread{ [] { loadShaders(); } }
	};

	for (auto& thread : threads)
	{
		thread.join();
	}

	labEntityID = createOneGameEntity({}, {}, nullptr).getId();
	fanEntityID = createOneGameEntity({ -10.47f, 5.93f, -6.47f }, {}, "fanScript").getId();
	planeEntityID = createOneGameEntity({ 0.f, 1.3f, -6.6f }, {}, "shipScript").getId();
	excaliburEntityID = createOneGameEntity({ -6.f, 0.f, 10.f }, { 0.f, math::pi, 0.f }, "excaliburScript").getId();

	createMaterial();
	id::idType materials[]{ defaultMaterialID };
	id::idType excaliburMaterials[]{ excaliburMaterialID };

	planeItemID = graphics::addRenderItem(planeEntityID, planeModelID, _countof(materials), &materials[0]);
	labItemID = graphics::addRenderItem(labEntityID, labModelID, _countof(materials), &materials[0]);
	fanItemID = graphics::addRenderItem(fanEntityID, fanModelID, _countof(materials), &materials[0]);
	excaliburItemID = graphics::addRenderItem(excaliburEntityID, excaliburModelID, _countof(excaliburMaterials), &excaliburMaterials[0]);

	renderItemMap[planeItemID] = planeEntityID;
	renderItemMap[labItemID] = labEntityID;
	renderItemMap[fanItemID] = fanEntityID;
	renderItemMap[excaliburItemID] = excaliburEntityID;
}

void destroyRenderItems()
{
	removeItem(planeEntityID, planeItemID, planeModelID);
	removeItem(labEntityID, labItemID, labModelID);
	removeItem(fanEntityID, fanItemID, fanModelID);
	removeItem(excaliburEntityID, excaliburItemID, excaliburModelID);

	//Remove materials.
	if (id::isValid(defaultMaterialID))
	{
		content::destroyResource(defaultMaterialID, content::assetType::material);
	}
	if (id::isValid(excaliburMaterialID))
	{
		content::destroyResource(excaliburMaterialID, content::assetType::material);
	}

	//Remove textures.
	for (id::idType id : textureIDs)
	{
		if (id::isValid(id))
		{
			content::destroyResource(id, content::assetType::texture);
		}
	}

	//Remove textures.
	for (id::idType id : textureIDs)
	{
		if (id::isValid(id))
		{
			content::destroyResource(id, content::assetType::texture);
		}
	}

	//Removes shaders and textures.
	if (id::isValid(vertexShaderID))
	{
		content::destroyShaderGroup(vertexShaderID);
	}
	if (id::isValid(pixelShaderID))
	{
		content::destroyShaderGroup(pixelShaderID);
	}
	if (id::isValid(texturedPixelShaderID))
	{
		content::destroyShaderGroup(texturedPixelShaderID);
	}
}

void getRenderItems(id::idType* items, [[maybe_unused]] u32 count)
{
	assert(count == 4);
	items[0] = planeItemID;
	items[1] = labItemID;
	items[2] = fanItemID;
	items[3] = excaliburItemID;
}