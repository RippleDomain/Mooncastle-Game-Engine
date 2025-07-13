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

//LNK4099 "PDB not found" warnings can be resolved either by installing FBX SDK PDBs (separate download) or by 
//disabling the specific warning in the linker options of the ContentTools project (Linker -> Command Line -> type in "/ignore:4099").

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
			}
			else if (node->GetLodGroup())
			{
				getLODGroup(node);
			}
			else 
			{
				getScene(node);
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

			if (getMeshData(fbxMesh, m))
			{
				meshes.emplace_back(m);
			}
		}

		getScene(node);
	}

	void FBXContext::getLODGroup(FbxNode* node)
	{
		assert(node);

		if (FbxLODGroup * lodGrp{ node->GetLodGroup() })
		{
			lodGroup lod{};
			lod.name = (node->GetName()[0] != '\0') ? node->GetName() : lodGrp->GetName();

			//Number of LODs is exclusive the base mesh (LOD 0).
			const i32 num_lods{ lodGrp->GetNumThresholds() };
			const i32 num_nodes{ node->GetChildCount() };

			assert(num_lods > 0 && num_nodes > 0);

			for (i32 i{ 0 }; i < num_nodes; ++i)
			{
				getMesh(node->GetChild(i), lod.meshes);

				if (lod.meshes.size() > 1 && lod.meshes.size() <= num_lods + 1 && lod.meshes.back().lodThreshold < 0.f)
				{
					FbxDistance threshold;
					lodGrp->GetThreshold((u32)lod.meshes.size() - 2, threshold);
					lod.meshes.back().lodThreshold = threshold.value() * sceneScale;
				}
			}

			if (lod.meshes.size()) scene->lodGroups.emplace_back(lod);
		}
	}

	bool FBXContext::getMeshData(FbxMesh * fbxMesh, mesh& m)
	{
		assert(fbxMesh);

		const i32 num_polys{ fbxMesh->GetPolygonCount() };
		if (num_polys <= 0)return false;

		const i32 numVertices{ fbxMesh->GetControlPointsCount() };
		FbxVector4* vertices{ fbxMesh->GetControlPoints() };
		const i32 numIndices{ fbxMesh->GetPolygonVertexCount() };
		i32* indices{ fbxMesh->GetPolygonVertices() };

		assert(numVertices > 0 && vertices && numIndices > 0 && indices);
		if (!(numVertices > 0 && vertices && numIndices > 0 && indices))return false;

		m.rawIndices.resize(numIndices);
		utl::vector vertexRef(numVertices, u32_invalid_id);

		for (i32 i{ 0 }; i < numIndices; ++i)
		{
			const u32 vIdx{ (u32)indices[i] };

			if (vertexRef[vIdx] != u32_invalid_id)
			{
				m.rawIndices[i] = vertexRef[vIdx];
			}
			else
			{
				FbxVector4 v = vertices[vIdx] * sceneScale;
				m.rawIndices[i] = (u32)m.positions.size();
				vertexRef[vIdx] = m.rawIndices[i];
				m.positions.emplace_back((f32)v[0], (f32)v[1], (f32)v[2]);
			}
		}

		assert(m.rawIndices.size() % 3 == 0);
		assert(num_polys > 0);

		FbxLayerElementArrayTemplate<i32>* mtlIndices;

		if (fbxMesh->GetMaterialIndices(&mtlIndices))
		{
			for (i32 i{ 0 }; i < num_polys; ++i)
			{
				const i32 mtlIndex{ mtlIndices->GetAt(i) };
				assert(mtlIndex >= 0);

				m.materialIndices.emplace_back((u32)mtlIndex);

				if (std::find(m.materialUsed.begin(), m.materialUsed.end(), (u32)mtlIndex) == m.materialUsed.end())
				{
					m.materialUsed.emplace_back((u32)mtlIndex);
				}
			}
		}

		const bool importNormals{ !sceneData->settings.calculateNormals };
		const bool importTangents{ !sceneData->settings.calculateTangents };

		if (importNormals)
		{
			FbxArray<FbxVector4> normals;

			//Calculates normals using FBX's built-in method, but only if no normal data is already there.
			if (fbxMesh->GenerateNormals() && fbxMesh->GetPolygonVertexNormals(normals) && normals.Size() > 0)
			{
				const i32 numNormals{ normals.Size() };

				for (i32 i{ 0 }; i < numNormals; ++i)
				{
					m.normals.emplace_back((f32)normals[i][0], (f32)normals[i][1], (f32)normals[i][2]);
				}
			}
			else
			{
				sceneData->settings.calculateNormals = true;
			}
		}

		if (importTangents)
		{
			FbxLayerElementArrayTemplate<FbxVector4>* tangents{ nullptr };

			//Calculates tangents using FBX's built-in method, but only if no tangent data is already there.
			if (fbxMesh->GenerateTangentsData() && fbxMesh->GetTangents(&tangents) && tangents && tangents->GetCount() > 0)
			{
				const i32 numTangents{ tangents->GetCount() };

				for (i32 i{ 0 }; i < numTangents; ++i)
				{
					FbxVector4 t{ tangents->GetAt(i) };
					m.tangents.emplace_back((f32)t[0], (f32)t[1], (f32)t[2], (f32)t[3]);
				}
			}
			else
			{
				sceneData->settings.calculateTangents = true;
			}
		}

		FbxStringList uvNames;
		fbxMesh->GetUVSetNames(uvNames);
		const i32 uvSetCount{ uvNames.GetCount() };

		m.uvSets.resize(uvSetCount);

		for (i32 i{ 0 }; i < uvSetCount; ++i)
		{
			FbxArray<FbxVector2> uvs;

			if (fbxMesh->GetPolygonVertexUVs(uvNames.GetStringAt(i), uvs))
			{
				const i32 num_uvs{ uvs.Size() };

				for (i32 j{ 0 }; j < num_uvs; ++j)
				{
					m.uvSets[i].emplace_back((f32)uvs[j][0], (f32)uvs[j][1]);
				}
			}
		}

		return true;
	}

	EDITOR_INTERFACE void ImportFbx(const char* file, sceneData* data)
	{
		assert(file && data);
		scene scene{};

		//Anything that involves the FBX SDK should be done in a single thread.
		{
			std::lock_guard lock{ fbxMutex };
			FBXContext fbxContext{ file, &scene, data };

			if (fbxContext.isValid())
			{
				fbxContext.getScene();
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