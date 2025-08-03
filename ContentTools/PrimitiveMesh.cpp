#include "PrimitiveMesh.h"
#include "Geometry.h"

namespace mooncastle::tools
{
	namespace
	{
		using primitiveMeshCreator = void(*)(scene&, const primitiveInitInfo& info);
		using namespace math;
		using namespace DirectX;

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

		mesh createUvSphere(const primitiveInitInfo& info) 
		{
			const u32 phiCount{ clamp(info.segments[axis::x], 3u, 64u) };
			const u32 thetaCount{ clamp(info.segments[axis::y], 2u, 64u) };
			const f32 thetaStep{ pi / thetaCount };
			const f32 phiStep{ tau / phiCount };
			const u32 numVertices{ 2 + phiCount * (thetaCount - 1) };
			const u32 numIndices{ 2 * 3 * phiCount + 2 * 3 * phiCount * (thetaCount - 2)};

			mesh m{};
			m.name = "uvSphere";
			m.positions.resize(numVertices);

			//Add the top vertex.
			u32 c{ 0 };
			m.positions[c++] = { 0.f, info.size.y, 0.f };

			for (u32 j{ 1 }; j <= (thetaCount - 1); ++j)
			{
				const f32 theta{ j * thetaStep };

				for (u32 i{ 0 }; i < phiCount; ++i)
				{
					const f32 phi{ i * phiStep };
					m.positions[c++] = {
						info.size.x* XMScalarSin(theta)* XMScalarCos(phi),
						info.size.y* XMScalarCos(theta),
						-info.size.z * XMScalarSin(theta) * XMScalarSin(phi)
					};
				}
			}

			//Add the bottom vertex.
			m.positions[c++] = { 0.f, -info.size.y, 0.f };
			assert(c == numVertices);

			c = 0;
			m.rawIndices.resize(numIndices);
			utl::vector<v2> uvs(numIndices);
			const f32 invThetaCount{ 1.f / thetaCount };
			const f32 invPhiCount{ 1.f / phiCount };

			//Indices for the top cap, connecting the north pole of the mesh to the first ring in the middle.
			for (u32 i{ 0 }; i < phiCount - 1; ++i)
			{
				uvs[c] = { (2 * i + 1) * 0.5f * invPhiCount, 1.f };
				m.rawIndices[c++] = 0;
				uvs[c] = { i * invPhiCount, 1.f - invThetaCount };
				m.rawIndices[c++] = i + 1;
				uvs[c] = { (i + 1) * invPhiCount, 1.f - invThetaCount };
				m.rawIndices[c++] = i + 2;
			}

			uvs[c] = { 1.f - 0.5f * invPhiCount, 1.f };
			m.rawIndices[c++] = 0;
			uvs[c] = { 1.f - invPhiCount, 1.f - invThetaCount };
			m.rawIndices[c++] = phiCount;
			uvs[c] = { 1.f, 1.f - invThetaCount };
			m.rawIndices[c++] = 1;

			//Indices for the section between the top and bottom rings.
			for (u32 j{ 0 }; j < (thetaCount - 2); ++j)
			{
				for (u32 i{ 0 }; i < (phiCount - 1); ++i)
				{
					const u32 index[4]{
						1 + i + j * phiCount,
						1 + i + (j + 1) * phiCount,
						1 + (i + 1) + (j + 1) * phiCount,
						1 + (i + 1) + j * phiCount
					};

					uvs[c] = { i * invPhiCount, 1.f - (j + 1) * invThetaCount };
					m.rawIndices[c++] = index[0];
					uvs[c] = { i * invPhiCount, 1.f - (j + 2) * invThetaCount };
					m.rawIndices[c++] = index[1];
					uvs[c] = { (i + 1) * invPhiCount, 1.f - (j + 2) * invThetaCount };
					m.rawIndices[c++] = index[2];

					uvs[c] = { i * invPhiCount, 1.f - (j + 1) * invThetaCount };
					m.rawIndices[c++] = index[0];
					uvs[c] = { (i + 1) * invPhiCount, 1.f - (j + 2) * invThetaCount };
					m.rawIndices[c++] = index[2];
					uvs[c] = { (i + 1) * invPhiCount, 1.f - (j + 1) * invThetaCount };
					m.rawIndices[c++] = index[3];
				}

				const u32 index[4]{
					phiCount + j * phiCount,
					phiCount + (j + 1) * phiCount,
					1 + (j + 1) * phiCount,
					1 + j * phiCount
				};

				uvs[c] = { 1.f - invPhiCount, 1.f - (j + 1) * invThetaCount };
				m.rawIndices[c++] = index[0];
				uvs[c] = { 1.f - invPhiCount, 1.f - (j + 2) * invThetaCount };
				m.rawIndices[c++] = index[1];
				uvs[c] = { 1.f, 1.f - (j + 2) * invThetaCount };
				m.rawIndices[c++] = index[2];

				uvs[c] = { 1.f - invPhiCount, 1.f - (j + 1) * invThetaCount };
				m.rawIndices[c++] = index[0];
				uvs[c] = { 1.f, 1.f - (j + 2) * invThetaCount };
				m.rawIndices[c++] = index[2];
				uvs[c] = { 1.f, 1.f - (j + 1) * invThetaCount };
				m.rawIndices[c++] = index[3];
			}

			//Indices for the bottom cap, connecting the south pole to the last ring.
			const u32 southPoleIndex{ (u32)m.positions.size() - 1 };

			for (u32 i{ 0 }; i < (phiCount - 1); ++i)
			{
				uvs[c] = { (2 * i + 1) * 0.5f * invPhiCount, 0.f };
				m.rawIndices[c++] = southPoleIndex;
				uvs[c] = { (i + 1) * invPhiCount, invThetaCount };
				m.rawIndices[c++] = southPoleIndex - phiCount + i + 1;
				uvs[c] = { i * invPhiCount, invThetaCount };
				m.rawIndices[c++] = southPoleIndex - phiCount + i;
			}

			uvs[c] = { 1.f - 0.5f * invPhiCount, 0.f };
			m.rawIndices[c++] = southPoleIndex;
			uvs[c] = { 1.f, invThetaCount };
			m.rawIndices[c++] = southPoleIndex - phiCount;
			uvs[c] = { 1.f - invPhiCount, invThetaCount };
			m.rawIndices[c++] = southPoleIndex - 1;

			assert(c == numIndices);

			m.uvSets.emplace_back(uvs);

			return m;
		}

		void createPlane(scene& scene, const primitiveInitInfo& info)
		{
			lodGroup lod{};
			lod.name = "plane";
			lod.meshes.emplace_back(createPlane(info));
			scene.lodGroups.emplace_back(lod);
		}

		void createCube(scene&, const primitiveInitInfo&)
		{

		}

		void createUvSphere(scene& scene, const primitiveInitInfo& info) 
		{
			lodGroup lod{};
			lod.name = "UvSphere";
			lod.meshes.emplace_back(createUvSphere(info));
			scene.lodGroups.emplace_back(lod);
		}

		void createIcoSphere(scene&, const primitiveInitInfo&)
		{

		}

		void createCylinder(scene&, const primitiveInitInfo&)
		{

		}

		void createCapsule(scene&, const primitiveInitInfo&)
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

		progression progression{};

		processScene(scene, data->settings, &progression);
		packData(scene, *data);
	}
}