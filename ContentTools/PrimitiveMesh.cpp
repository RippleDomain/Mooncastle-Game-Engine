#include "PrimitiveMesh.h"
#include "Geometry.h"

namespace mooncastle::tools
{
	namespace
	{
		using primitiveMeshCreator = void(*)(scene&, const primitiveInitInfo& info);
		using namespace math;

		void createPlane(scene& scene, const primitiveInitInfo& info);
		void createCube(scene& scene, const primitiveInitInfo& info);
		void createUvSphere(scene& scene, const primitiveInitInfo& info);
		void createIcoSphere(scene& scene, const primitiveInitInfo& info);
		void createCylinder(scene& scene, const primitiveInitInfo& info);
		void createCapsule(scene& scene, const primitiveInitInfo& info);

		primitiveMeshCreator creators[]
		{
			createPlane,
			createCube,
			createUvSphere,
			createIcoSphere,
			createCylinder,
			createCapsule
		};

		struct axis 
		{
			enum : u32 
			{
				x = 0,
				y = 1,
				z = 2
			};
		};

		mesh createPlane(const primitiveInitInfo& info,
			u32 horizontalIndex = axis::x, u32 verticalIndex = axis::z, bool flipWinding = false,
			v3 offset = { -0.5f, 0.f, -0.5f }, v2 uRange = { 0.f, 1.f }, v2 vRange = { 0.f, 1.f })
		{
			assert(horizontalIndex < 3 && verticalIndex < 3);
			assert(horizontalIndex != verticalIndex);

			const u32 horizontalCount{ clamp(info.segments[horizontalIndex], 1u, 10u) };
			const u32 verticalCount{ clamp(info.segments[verticalIndex], 1u, 10u) };
			const f32 horizontalStep{ 1.f / horizontalCount };
			const f32 verticalStep{ 1.f / verticalCount };
			const f32 uStep{ (uRange.y - uRange.x) / horizontalCount };
			const f32 vStep{ (vRange.y - vRange.x) / verticalCount };

			mesh m{};

			utl::vector<v2> uvs;

			for (u32 j{ 0 }; j <= verticalCount; ++j)
			{
				for (u32 i{ 0 }; i <= horizontalCount; ++i)
				{
					v3 position{ offset };
					f32* const asArray{ &position.x };
					asArray[horizontalIndex] += i * horizontalStep;
					asArray[verticalIndex] += j * verticalStep;
					m.positions.emplace_back(position.x * info.size.x, position.y * info.size.y, position.z * info.size.z);

					v2 uv{ uRange.x, 1.f - vRange.x };
					uv.x += i * uStep;
					uv.y -= j * vStep;
					/*v2 uv{ 0, 1.f };
					uv.x += (i % 2);
					uv.y -= (j % 2);*/
					uvs.emplace_back(uv);
				}
			}

			assert(m.positions.size() == (((u64)horizontalCount + 1) * ((u64)verticalCount + 1)));

			const u32 rowLength{ horizontalCount + 1 }; //Number of vertices in a row
			for (u32 j{ 0 }; j < verticalCount; ++j)
			{
				u32 k{ 0 };
				for (u32 i{ 0 }; i < horizontalCount; ++i)
				{
					const u32 index[4]
					{
						i + j * rowLength,
						i + (j + 1) * rowLength,
						(i + 1) + j * rowLength,
						(i + 1) + (j + 1) * rowLength
					};

					m.rawIndices.emplace_back(index[0]);
					m.rawIndices.emplace_back(index[flipWinding ? 2 : 1]);
					m.rawIndices.emplace_back(index[flipWinding ? 1 : 2]);

					m.rawIndices.emplace_back(index[2]);
					m.rawIndices.emplace_back(index[flipWinding ? 3 : 1]);
					m.rawIndices.emplace_back(index[flipWinding ? 1 : 3]);

					++k;
				}
			}

			const u32 numIndices{ 3 * 2 * horizontalCount * verticalCount };
			assert(m.rawIndices.size() == numIndices);

			m.uvSets.resize(1);

			for (u32 i{ 0 }; i < numIndices; ++i)
			{
				m.uvSets[0].emplace_back(uvs[m.rawIndices[i]]);
			}

			return m;
		}

		void createPlane(scene& scene, const primitiveInitInfo& info)
		{
			lodGroup lod{};
			lod.name = "plane";
			lod.meshes.emplace_back(createPlane(info));
			scene.lodGroups.emplace_back(lod);
		}

		void createCube(scene& scene, const primitiveInitInfo& info)
		{

		}

		void createUvSphere(scene& scene, const primitiveInitInfo& info) 
		{

		}

		void createIcoSphere(scene& scene, const primitiveInitInfo& info)
		{

		}

		void createCylinder(scene& scene, const primitiveInitInfo& info)
		{

		}

		void createCapsule(scene& scene, const primitiveInitInfo& info)
		{

		}

		static_assert(_countof(creators) == primitiveMeshType::count);
	}

	EDITOR_INTERFACE void CreatePrimitiveMesh(sceneData* data, primitiveInitInfo* info) 
	{
		assert(data && info);
		assert(info->type < primitiveMeshType::count);
		scene scene{};

		creators[info->type](scene, *info);

		data->settings.calculateNormals = 1;
		processScene(scene, data->settings);
		packData(scene, *data);
	}
}