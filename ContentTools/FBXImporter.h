#pragma once

#include "ToolsCommon.h"
#include <fbxsdk.h>

namespace mooncastle::tools
{
	struct sceneData;
	struct scene;
	struct mesh;
	struct geometryImportSettings;

	class FBXContext
	{
	public:
		FBXContext(const char* file, scene* newScene, sceneData* newData, progression *const newProgression)
			: scene{ newScene }, sceneData{ newData }, currentProgression{ newProgression }
		{
			assert(file && scene && sceneData && currentProgression);

			if (initializeFBX())
			{
				loadFBXFile(file);
				assert(isValid());
			}
		}

		~FBXContext()
		{
			fbxScene->Destroy();
			fbxManager->Destroy();
			ZeroMemory(this, sizeof(FBXContext));
		}

		void getScene(FbxNode* root = nullptr);
		constexpr bool isValid() const { return fbxManager && fbxScene; }
		constexpr f32 getSceneScale() const { return sceneScale; }
		constexpr progression* getProgression() const { return currentProgression; }

	private:
		bool initializeFBX();
		void loadFBXFile(const char* file);
		void getMeshes(FbxNode* node, utl::vector<mesh>& meshes, u32 lodId, f32 lodThreshold);
		void getMesh(FbxNodeAttribute* attribute, utl::vector<mesh>& meshes, u32 lodId, f32 lodThreshold);
		void getLODGroup(FbxNodeAttribute* attribute);
		bool getMeshData(FbxMesh* fbxMesh, mesh& m);

		scene*				scene{ nullptr };
		sceneData*			sceneData{ nullptr };
		FbxManager*			fbxManager{ nullptr };
		FbxScene*			fbxScene{ nullptr };
		progression*		currentProgression{ nullptr };
		f32					sceneScale{ 1.0f };
	};
}