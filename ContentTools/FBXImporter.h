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
		FBXContext(const char* file, scene* newScene, sceneData* newData) :scene{ newScene }, sceneData{ newData }
		{
			assert(file && scene && sceneData);

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

	private:
		bool initializeFBX();
		void loadFBXFile(const char* file);
		void getMesh(FbxNode* node, utl::vector<mesh>& meshes);
		void getLODGroup(FbxNode* node);

		scene*				scene{ nullptr };
		sceneData*			sceneData{ nullptr };
		FbxManager*			fbxManager{ nullptr };
		FbxScene*			fbxScene{ nullptr };
		f32					sceneScale{ 1.0f };
	};
}