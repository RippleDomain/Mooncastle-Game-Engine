#include <filesystem>
#include "CommonHeaders.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "Components/Entity.h"
#include "Components/Geometry.h"
#include "../EngineDLL/ShaderCompilation.h"
#include "Test.h"
#include "../ContentTools/Geometry.h"

#if TEST_RENDERER

using namespace mooncastle;

gameEntity::entity createOneGameEntity(math::v3 position, math::v3 rotation, geometry::initInfo* geometryInfo, const char* scriptName);
void removeGameEntity(gameEntity::entityId);
bool readFile(std::filesystem::path, std::unique_ptr<u8[]>&, u64&);

namespace
{
	id::idType labModelID{ id::invalidId };
	id::idType fanModelID{ id::invalidId };
	id::idType planeModelID{ id::invalidId };
	id::idType excaliburModelID{ id::invalidId };
	//id::idType botModelID{ id::invalidId };
	id::idType sphereModelID{ id::invalidId };

	gameEntity::entityId labEntityID{ id::invalidId };
	gameEntity::entityId fanEntityID{ id::invalidId };
	gameEntity::entityId planeEntityID{ id::invalidId };
	gameEntity::entityId excaliburEntityID{ id::invalidId };
	//gameEntity::entityId botEntityID{ id::invalidId };
	gameEntity::entityId sphereEntityIDs[12];

	struct textureUsage
	{
		enum usage : u32 
		{
			ambientOcclusion = 0,
			baseColor,
			emissive,
			//metalRough,
			metal,
			roughness,
			normal,
			count
		};
	};

	id::idType textureIDs[textureUsage::count];

	id::idType iblBRDFLUTID{ id::invalidId };
	id::idType iblDiffuseID{ id::invalidId };
	id::idType iblSpecularID{ id::invalidId };

	id::idType vertexShaderID{ id::invalidId };
	id::idType pixelShaderID{ id::invalidId };
	id::idType texturedPixelShaderID{ id::invalidId };

	id::idType defaultMaterialID{ id::invalidId };
	id::idType excaliburMaterialID{ id::invalidId };
	//id::idType botMaterialID{ id::invalidId };

	id::idType pbrMaterialIDs[12];

	graphics::light iblLight{};

	[[nodiscard]] id::idType loadAsset(const char* path, content::assetType::type type)
	{
		std::unique_ptr<u8[]> buffer;

		u64 size{ 0 };
		readFile(path, buffer, size);

		const id::idType assetID{ content::createResource(buffer.get(), type) };
		assert(id::isValid(assetID));

		return assetID;
	}

	//Loading the test model.
	[[nodiscard]] id::idType loadModel(const char* path)
	{
		return loadAsset(path, content::assetType::mesh);
	}

	//Loading the test texture.
	[[nodiscard]] id::idType loadTexture(const char* path)
	{
		return loadAsset(path, content::assetType::texture);
	}

	void loadShaders()
	{
		shaderFileInfo info{};
		info.fileName = "TestShader.hlsl";
		info.function = "TestShaderVS";
		info.type = graphics::shaderType::vertex;

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
		info.type = graphics::shaderType::pixel;

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
		assert(id::isValid(vertexShaderID) && id::isValid(pixelShaderID) && id::isValid(texturedPixelShaderID));

		graphics::materialInitInfo info{};

		info.shaderIDs[graphics::shaderType::vertex] = vertexShaderID;
		info.shaderIDs[graphics::shaderType::pixel] = pixelShaderID;
		info.type = graphics::materialType::opaque;

		defaultMaterialID = content::createResource(&info, content::assetType::material);

		memset(pbrMaterialIDs, 0xff, sizeof(pbrMaterialIDs));

		math::v2 metalRough[_countof(pbrMaterialIDs)]
		{
			{0.f, 0.0f}, {0.f, 0.2f}, {0.f, 0.4f}, {0.f, 0.6f}, {0.f, 0.8f}, {0.f, 1.f},
			{1.f, 0.0f}, {1.f, 0.2f}, {1.f, 0.4f}, {1.f, 0.6f}, {1.f, 0.8f}, {1.f, 1.f},
		};

		graphics::materialSurface& s{ info.surface };
		s.baseColor = { 0.5f, 0.5f, 0.5f, 1.f };

		for (u32 i{ 0 }; i < _countof(pbrMaterialIDs); ++i)
		{
			s.metallic = metalRough[i].x;
			s.roughness = metalRough[i].y;
			pbrMaterialIDs[i] = content::createResource(&info, content::assetType::material);
		}

		info.shaderIDs[graphics::shaderType::pixel] = texturedPixelShaderID;
		info.textureCount = textureUsage::count;
		info.textureIDs = &textureIDs[0];

		excaliburMaterialID = content::createResource(&info, content::assetType::material);
		//botMaterialID = content::createResource(&info, content::assetType::material);
	}

	void createIBLLight()
	{
		graphics::lightInitInfo info{};
		info.entityID = 0;
		info.type = graphics::light::ambient;
		info.ambientParams.brdfLUTTextureID = iblBRDFLUTID;
		info.ambientParams.diffuseTextureID = iblDiffuseID;
		info.ambientParams.specularTextureID = iblSpecularID;

		iblLight = graphics::createLight(info);
	}

	void removeModel(id::idType modelID)
	{
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
	//assert(std::filesystem::exists("..\\..\\x64\\botModel.model"));

	memset(&textureIDs[0], 0xff, sizeof(id::idType) * _countof(textureIDs));

	std::thread threads[]
	{
		//Textures.
		std::thread{ [] { textureIDs[textureUsage::ambientOcclusion] = loadTexture("..\\..\\x64\\excaliburAO.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::baseColor] = loadTexture("..\\..\\x64\\excaliburBaseColor.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::emissive] = loadTexture("..\\..\\x64\\excaliburEmissive.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::metal] = loadTexture("..\\..\\x64\\excaliburMetal.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::roughness] = loadTexture("..\\..\\x64\\excaliburRoughness.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::normal] = loadTexture("..\\..\\x64\\excaliburNormal.texture"); }},

		/*std::thread{ [] { textureIDs[textureUsage::ambientOcclusion] = loadTexture("..\\..\\x64\\botAO.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::baseColor] = loadTexture("..\\..\\x64\\botBaseColor.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::emissive] = loadTexture("..\\..\\x64\\botEmissive.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::metalRough] = loadTexture("..\\..\\x64\\botMetalRough.texture"); }},
		std::thread{ [] { textureIDs[textureUsage::normal] = loadTexture("..\\..\\x64\\botNormal.texture"); }},*/

		std::thread{ [] { iblBRDFLUTID = loadTexture("..\\..\\x64\\IBL\\brdfLUT.texture"); } },
		std::thread{ [] { iblDiffuseID = loadTexture("..\\..\\x64\\IBL\\Set1\\diffuse.texture"); } },
		std::thread{ [] { iblSpecularID = loadTexture("..\\..\\x64\\IBL\\Set1\\specular.texture"); } },

		//Models.
		std::thread{ [] { labModelID = loadModel("..\\..\\x64\\labModel.model"); } },
		std::thread{ [] { fanModelID = loadModel("..\\..\\x64\\fanModel.model"); } },
		std::thread{ [] { planeModelID = loadModel("..\\..\\x64\\planeModel.model"); } },
		std::thread{ [] { excaliburModelID = loadModel("..\\..\\x64\\excaliburModel.model"); } },
		//std::thread{ [] { botModelID = loadModel("..\\..\\x64\\botModel.model"); } },
		std::thread{ [] { sphereModelID = loadModel("..\\..\\x64\\sphereModel.model"); } },
		std::thread{ [] { loadShaders(); } }
	};

	for (auto& thread : threads)
	{
		thread.join();
	}

	createIBLLight();

	createMaterial();
	id::idType materials[]{ defaultMaterialID };
	id::idType excaliburMaterials[]{ excaliburMaterialID };
	//id::idType botMaterials[]{ botMaterialID, botMaterialID };

	geometry::initInfo geometryInfo{};
	geometryInfo.materialCount = _countof(materials);
	geometryInfo.materialIDs = &materials[0];

	geometryInfo.geometryContentID = labModelID;
	labEntityID = createOneGameEntity({}, {}, &geometryInfo, nullptr).getId();

	geometryInfo.geometryContentID = fanModelID;
	fanEntityID = createOneGameEntity({ -10.47f, 5.93f, -6.7f }, {}, &geometryInfo, "fanScript").getId();

	geometryInfo.geometryContentID = planeModelID;
	planeEntityID = createOneGameEntity({ 0.f, 1.3f, -6.6f }, {}, &geometryInfo, "shipScript").getId();

	geometryInfo.geometryContentID = excaliburModelID;
	geometryInfo.materialCount = _countof(excaliburMaterials);
	geometryInfo.materialIDs = &excaliburMaterials[0];
	excaliburEntityID = createOneGameEntity({ -6.f, 0.f, 10.f }, { 0.f, math::pi, 0.f }, &geometryInfo, "excaliburScript").getId();

	/*geometryInfo.geometryContentID = botModelID;
	geometryInfo.materialCount = _countof(botMaterials);
	geometryInfo.materialIDs = &botMaterials[0];
	botEntityID = createOneGameEntity({ -6.f, 0.f, 10.f }, { 0.f, math::pi, 0.f }, &geometryInfo, "rotatorScript").getId();*/

	geometryInfo.geometryContentID = sphereModelID;
	geometryInfo.materialCount = 1;

	for (u32 i{ 0 }; i < _countof(sphereEntityIDs); ++i)
	{
		id::idType id{ pbrMaterialIDs[i] };
		id::idType sphere_mtls[]{ id };
		geometryInfo.materialIDs = &sphere_mtls[0];

		const f32 x{ -6.f + i % 6 };
		const f32 y{ (i < 6) ? 7.f : 5.5f };
		const f32 z = x;

		sphereEntityIDs[i] = createOneGameEntity({ x, y, z }, {}, &geometryInfo, nullptr).getId();
	}
}

void destroyRenderItems()
{
	removeGameEntity(labEntityID);
	removeGameEntity(fanEntityID);
	removeGameEntity(planeEntityID);
	removeGameEntity(excaliburEntityID);
	/*removeGameEntity(botEntityID);*/

	for (u32 i{ 0 }; i < _countof(sphereEntityIDs); ++i)
	{
		removeGameEntity(sphereEntityIDs[i]);
	}

	removeModel(labModelID);
	removeModel(fanModelID);
	removeModel(planeModelID);
	removeModel(excaliburModelID);
	//removeModel(botModelID);
	removeModel(sphereModelID);

	if (iblLight.isValid())
	{
		graphics::removeLight(iblLight.getID(), 0);
	}

	//Remove materials.
	if (id::isValid(defaultMaterialID))
	{
		content::destroyResource(defaultMaterialID, content::assetType::material);
	}
	if (id::isValid(excaliburMaterialID))
	{
		content::destroyResource(excaliburMaterialID, content::assetType::material);
	}
	/*if (id::isValid(botMaterialID))
	{
		content::destroyResource(botMaterialID, content::assetType::material);
	}*/

	for (id::idType id : pbrMaterialIDs)
	{
		if (id::isValid(id))
		{
			content::destroyResource(id, content::assetType::material);
		}
	}

	if (id::isValid(iblBRDFLUTID))
	{
		content::destroyResource(iblBRDFLUTID, content::assetType::texture);
	}

	if (id::isValid(iblDiffuseID))
	{
		content::destroyResource(iblDiffuseID, content::assetType::texture);
	}

	if (id::isValid(iblSpecularID))
	{
		content::destroyResource(iblSpecularID, content::assetType::texture);
	}

	//Remove textures.
	for (id::idType id : textureIDs)
	{
		if (id::isValid(id))
		{
			content::destroyResource(id, content::assetType::texture);
		}
	}

	//Removes shaders.
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

#endif