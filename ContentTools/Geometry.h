#pragma once

#include <string>

#include "ToolsCommon.h"

namespace mooncastle::tools 
{
	struct mesh
	{
		//Initial mesh data
		utl::vector<math::v3>                    positions;
		utl::vector<math::v3>                    normals;
		utl::vector<math::v4>                    tangents;
		utl::vector<utl::vector<math::v2>>       uvSets;
		utl::vector<u32>                         rawIndices;

		//Intermediate mesh data

		//Output mesh data

	};

	struct lodGroup
	{
		std::string        name;
		utl::vector<mesh>  meshes;
	};

	struct scene
	{
		std::string            name;
		utl::vector<lodGroup>  lodGroups;
	};
	struct geometryImportSettings 
	{
		f32 smoothingAngle;
		u8  calculateNormals;
		u8  calculateTangents;
		u8  reverseHandedness;
		u8  importEmbeddedTextures;
		u8  importAnimations;
	};

	struct sceneData 
	{
		u8*                      buffer;
		u32                      bufferSize;
		geometryImportSettings   settings;
	};
}