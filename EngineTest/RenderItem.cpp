#include <filesystem>
#include "CommonHeaders.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "Components/Entity.h"
#include "ShaderCompilation.h"

using namespace mooncastle;

bool readFile(std::filesystem::path, std::unique_ptr<u8[]>&, u64&);

namespace
{
	id::idType modelId{ id::invalidId };
	id::idType vertexShaderId{ id::invalidId };
	id::idType pixelShaderId{ id::invalidId };
	id::idType materialId{ id::invalidId };

	std::unordered_map<id::idType, id::idType> renderItemMap;

	void loadModel()
	{
		std::unique_ptr<u8[]> model;
		u64 size{ 0 };
		readFile("..\\..\\EngineTest\\model.model", model, size);

		modelId = content::createResource(model.get(), content::assetType::mesh);
		assert(id::isValid(modelId));
	}

	void loadShaders()
	{
		shaderFileInfo info{};
		info.fileName = "TestShader.hlsl";
		info.function = "TestShaderVS";
		info.type = shaderType::vertex;

		const char* shaderPath{ "..\\..\\EngineTest\\" };

		auto vertexShader = compileShader(info, shaderPath);

		assert(vertexShader.get());

		info.function = "TestShaderPS";
		info.type = shaderType::pixel;

		auto pixelShader = compileShader(info, shaderPath);

		assert(pixelShader.get());

		vertexShaderId = content::createShader(vertexShader.get());
		pixelShaderId = content::createShader(pixelShader.get());
	}

	void createMaterial()
	{
		graphics::materialInitInfo info{};

		info.shaderIDs[graphics::shaderType::vertex] = vertexShaderId;
		info.shaderIDs[graphics::shaderType::pixel] = pixelShaderId;
		info.type = graphics::materialType::opaque;

		materialId = content::createResource(&info, content::assetType::material);
	}
}

id::idType createRenderItem(id::idType entityID)
{
	//Loads a model, pretend it belongs to entityID.
	auto first = std::thread([] { loadModel(); });

	//Loads a material:
	//1) Loads textures.
	//2) Loads shaders for that material.
	auto second = std::thread([] { loadShaders(); });

	first.join();
	second.join();

	//Add a render item using the model and its materials.
	createMaterial();
	id::idType materials[]{ materialId, materialId, materialId, materialId };

	id::idType itemID{ graphics::addRenderItem(0, modelId, _countof(materials), &materials[0])};
	renderItemMap[itemID] = entityID;

	return itemID;
}

void destroyRenderItem(id::idType itemID)
{
	//Removes the render item from engine (also the game entity).
	if (id::isValid(itemID))
	{
		graphics::removeRenderItem(itemID);

		auto pair = renderItemMap.find(itemID);

		if (pair != renderItemMap.end())
		{
			gameEntity::remove(gameEntity::entityId{ pair->second });
		}
	}

	//Remove material.
	if (id::isValid(materialId))
	{
		content::destroyResource(materialId, content::assetType::material);
	}

	//Removes shaders and textures.
	if (id::isValid(vertexShaderId))
	{
		content::destroyShader(vertexShaderId);
	}
	if (id::isValid(pixelShaderId))
	{
		content::destroyShader(pixelShaderId);
	}

	//Removes model.
	if (id::isValid(modelId))
	{
		content::destroyResource(modelId, content::assetType::mesh);
	}	
}