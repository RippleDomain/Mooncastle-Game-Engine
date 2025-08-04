#pragma once

#include <string>

#include "ToolsCommon.h"

namespace mooncastle::tools 
{
	struct vertex
	{
		math::v4       tangent{};
		math::v4       jointWeights{};
		math::u32v4    jointIndices{ u32_invalid_id, u32_invalid_id, u32_invalid_id, u32_invalid_id };
		math::v3       position{};
		math::v3       normal{};
		math::v2       uv{};
		u8             red{}, green{}, blue{};
		u8             pad;
	};

	namespace elements
	{
		struct elementTypes
		{
			enum type : u32
			{
				positionOnly = 0x00,
				staticNormal = 0x01,
				staticNormalTexture = 0x03,
				staticColor = 0x04,
				skeletal = 0x08,
				skeletalColor = skeletal | staticColor,
				skeletalNormal = skeletal | staticNormal,
				skeletalNormalColor = skeletalNormal | staticColor,
				skeletalNormalTexture = skeletal | staticNormalTexture,
				skeletalNormalTextureColor = skeletalNormalTexture | staticColor,
			};
		};

		struct staticColor
		{
			u8  color[3];
			u8  pad;
		};

		struct staticNormal
		{
			u8   color[3];
			u8   tSign; //Bit 0: tangent handidness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
			u16  normal[2];
		};

		struct staticNormalTexture
		{
			u8        color[3];
			u8        tSign; //Bit 0: tangent handidness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
			u16       normal[2];
			u16       tangent[2];
			math::v2  uv;
		};

		struct skeletal
		{
			u8   jointWeights[3]; //Normalized joint weights for up to 4 joints.
			u8   pad;
			u16  jointIndices[4];
		};

		struct skeletalColor
		{
			u8   jointWeights[3]; //Normalized joint weights for up to 4 joints.
			u8   pad;
			u16  jointIndices[4];
			u8   color[3];
			u8   pad2;
		};

		struct skeletalNormal
		{
			u8   jointWeights[3]; //Normalized joint weights for up to 4 joints.
			u8   tSign; //Bit 0: tangent handidness, bit 1: tangent.z sign, bit 2: normal.z sign (0 means -1, 1 means +1)
			u16  jointIndices[4];
			u16  normal[2];
		};

		struct skeletalNormalColor
		{
			u8   jointWeights[3]; //Normalized joint weights for up to 4 joints.
			u8   tSign; //Bit 0: tangent handidness, bit 1: tangent.z sign, bit 2: normal.z sign (0 means -1, 1 means +1)
			u16  jointIndices[4];
			u16  normal[2];
			u8   color[3];
			u8   pad;
		};

		struct skeletalNormalTexture
		{
			u8        jointWeights[3]; //Normalized joint weights for up to 4 joints.
			u8   tSign; //Bit 0: tangent handidness, bit 1: tangent.z sign, bit 2: normal.z sign (0 means -1, 1 means +1)
			u16       jointIndices[4];
			u16       normal[2];
			u16       tangent[2];
			math::v2  uv;
		};

		struct skeletalNormalTextureColor
		{
			u8        jointWeights[3]; //Normalized joint weights for up to 4 joints.
			u8   tSign; //Bit 0: tangent handidness, bit 1: tangent.z sign, bit 2: normal.z sign (0 means -1, 1 means +1)
			u16       jointIndices[4];
			u16       normal[2];
			u16       tangent[2];
			math::v2  uv;
			u8        color[3];
			u8        pad;
		};
	}

	struct mesh
	{
		//Initial mesh data
		utl::vector<math::v3>                    positions;
		utl::vector<math::v3>                    normals;
		utl::vector<math::v4>                    tangents;
		utl::vector<math::v3>                    colors;
		utl::vector<utl::vector<math::v2>>       uvSets;
		utl::vector<u32>                         rawIndices;
		utl::vector<u32>                         materialIndices;
		utl::vector<u32>                         materialUsed;

		//Intermediate mesh data
		utl::vector<vertex>                      vertices;
		utl::vector<u32>                         indices;

		//Output mesh data
		std::string                              name;
		elements::elementTypes::type             elementType;
		utl::vector<u8>                          positionBuffer;
		utl::vector<u8>                          elementBuffer;
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
		u8  coalesceMeshes;
	};

	struct sceneData 
	{
		u8*                      buffer;
		u32                      bufferSize;
		geometryImportSettings   settings;
	};

	void processScene(scene& scene, const geometryImportSettings& settings, progression *const progression);
	void packData(const scene& scene, sceneData& data);
	bool coalesceMeshes(const lodGroup& lod, mesh& combinedMesh, progression *const progression);
}