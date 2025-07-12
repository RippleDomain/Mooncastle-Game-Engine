#include "FBXImporter.h"
#include "Geometry.h"

//If you get any compilation or linker errors than make sure that:
//1) FBX SDK 2020.2 or later is installed on your system.
//2) The include path to fbxsdk.h is added to your "Additional Include Directories" (in the compiler settings).
//3) The library paths in the following section point to the correct location.
#if _DEBUG
#pragma comment (lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.3.2\\lib\\vs2019\\x64\\debug\\libfbxsdk-md.lib")
#pragma comment (lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.3.2\\lib\\vs2019\\x64\\debug\\libxml2-md.lib")
#pragma comment (lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.3.2\\lib\\vs2019\\x64\\debug\\zlib-md.lib")
#else
#pragma comment (lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.3.2\\lib\\vs2019\\x64\\release\\libfbxsdk-md.lib")
#pragma comment (lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.3.2\\lib\\vs2019\\x64\\release\\libxml2-md.lib")
#pragma comment (lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.3.2\\lib\\vs2019\\x64\\release\\zlib-md.lib")
#endif


namespace mooncastle::tools
{
	namespace
	{
		std::mutex fbxMutex;
	}

	bool FBXContext::initializeFBX()
	{
		assert(!isValid());

		fbxManager = FbxManager::Create();

		if (!fbxManager)
		{
			return false;
		}

		FbxIOSettings* ios{ FbxIOSettings::Create(fbxManager, IOSROOT) };
		assert(ios);
		fbxManager->SetIOSettings(ios);

		return true;
	}

	void FBXContext::loadFBXFile(const char* file)
	{
		assert(fbxManager && !fbxScene);

		fbxScene = FbxScene::Create(fbxManager, "Importer Scene");

		if (!fbxScene)
		{
			return;
		}

		FbxImporter* importer{ FbxImporter::Create(fbxManager, "Importer") };

		if (!(importer && importer->Initialize(file, -1, fbxManager->GetIOSettings()) && importer->Import(fbxScene)))
		{
			return;
		}

		importer->Destroy();

		sceneScale = (f32)fbxScene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m);
	}

	void FBXContext::getScene(FbxNode* root)
	{
		assert(isValid());

		if (!root)
		{
			root = fbxScene->GetRootNode();

			if (!root)return;
		}

		const i32 numNodes{ root->GetChildCount() };

		for (i32 i = 0; i < numNodes; i++)
		{
			FbxNode* node{ root->GetChild(i) };

			if (!node)continue;

			if (node->GetMesh())
			{
				lodGroup lod{};
				getMesh(node, lod.meshes);

				if (lod.meshes.size())
				{
					lod.name = lod.meshes[0].name;
					scene->lodGroups.emplace_back(lod);
				}
				else if (node->GetLodGroup())
				{
					getLODGroup(node);
				}
			}
		}
	}

	void FBXContext::getMesh(FbxNode* node, utl::vector<mesh>& meshes)
	{
		assert(node);

		if (FbxMesh* fbxMesh{ node->GetMesh() })
		{
			if (fbxMesh->RemoveBadPolygons() < 0) return;

			FbxGeometryConverter gc{ fbxManager };
			fbxMesh = static_cast<FbxMesh*>(gc.Triangulate(fbxMesh, true));

			if (!fbxMesh || fbxMesh->RemoveBadPolygons() < 0) return;

			mesh m;
			m.lodId = (u32)meshes.size();
			m.lodThreshold = -1.f;
			m.name = (node->GetName()[0] != '\0') ? node->GetName() : fbxMesh->GetName();
		}
	}

	void FBXContext::getLODGroup(FbxNode* node)
	{

	}

	EDITOR_INTERFACE void ImportFbx(const char* file, sceneData* data)
	{
		assert(file && data);
		scene scene{};

		//Anything that involves the FBX SDK should be done in a single thread.
		{
			std::lock_guard lock{ fbxMutex };
			FBXContext fbx_contex{ file, &scene, data };

			if (fbx_contex.isValid())
			{

			}
			else
			{
				return;
			}
		}

		processScene(scene, data->settings);
		packData(scene, *data);
	}
}