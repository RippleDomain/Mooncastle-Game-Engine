#pragma once

#include <string>

#include "ToolsCommon.h"

namespace mooncastle::tools 
{
	namespace packedVertex
	{
		struct vertexStatic
		{
			math::v3	position;
			u8			reserved[3];
			u8			tSign; //Bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
			u16			normal[2];
			u16			tangent[2];
			math::v2	uv;
		};
	}

	struct vertex
	{
		math::v4 tangent{};
		math::v3 position{};
		math::v3 normal{};
		math::v2 uv{};
	};

	struct mesh
	{
		//Initial mesh data
		utl::vector<math::v3>                    positions;
		utl::vector<math::v3>                    normals;
		utl::vector<math::v4>                    tangents;
		utl::vector<utl::vector<math::v2>>       uvSets;
		utl::vector<u32>                         rawIndices;
		utl::vector<u32>                         materialIndices;
		utl::vector<u32>                         materialUsed;

		//Intermediate mesh data
		utl::vector<vertex>                      vertices;
		utl::vector<u32>                         indices;

		//Output mesh data
		std::string                              name;
		utl::vector<packedVertex::vertexStatic>  packedVerticesStatic;
		f32                                      lodThreshold{ -1.f };
		u32                                      lodId{ u32_invalid_id };

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

	void processScene(scene& scene, const geometryImportSettings& settings);
	void packData(const scene& scene, sceneData& data);
}