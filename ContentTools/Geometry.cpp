#include "Geometry.h"

namespace mooncastle::tools
{
	namespace
	{
        using namespace math;
        using namespace DirectX;

        void recalculateNormals(mesh& m)
        {
            const u32 numIndices{ (u32)m.rawIndices.size() };
            m.normals.reserve(numIndices);

            for (u32 i{ 0 }; i < numIndices; ++i)
            {
                const u32 i0{ m.rawIndices[i++] };
                const u32 i1{ m.rawIndices[i++] };
                const u32 i2{ m.rawIndices[i++] };

                XMVECTOR v0{ XMLoadFloat3(&m.positions[i0]) };
                XMVECTOR v1{ XMLoadFloat3(&m.positions[i1]) };
                XMVECTOR v2{ XMLoadFloat3(&m.positions[i2]) };

                XMVECTOR e0{ v1 - v0 };
                XMVECTOR e1{ v2 - v0 };
                XMVECTOR n{ XMVector3Normalize(XMVector3Cross(e0, e1)) };

                XMStoreFloat3(&m.normals[i], n);
                m.normals[i - 1] = m.normals[i];
                m.normals[i - 2] = m.normals[i];
            }
        }

        void processNormals(mesh& m, f32 smoothingAngle)
        {
            const f32 cosAlpha{ XMScalarCos(pi - smoothingAngle * pi / 180.f) };
            const bool isHardEdge{ XMScalarNearEqual(smoothingAngle, 180.f, epsilon) };
            const bool isSoftEdge{ XMScalarNearEqual(smoothingAngle, 0.f, epsilon) };
            const u32 numIndices{ (u32)m.rawIndices.size() };
            const u32 numVertices{ (u32)m.positions.size() };
            assert(numIndices && numVertices);

            m.indices.resize(numIndices);

            utl::vector<utl::vector<u32>> idxRef(numVertices);

            for (u32 i{ 0 }; i < numIndices; ++i) 
            {
                idxRef[m.rawIndices[i]].emplace_back(i);
            }

            for (u32 i{ 0 }; i < numVertices; ++i)
            {
                auto& refs{ idxRef[i] };
                u32 numRefs{ (u32)refs.size() };

                for (u32 j{ 0 }; j < numRefs; ++j)
                {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    vertex& v{ m.vertices.emplace_back() };
                    v.position = m.positions[m.rawIndices[refs[j]]];

                    XMVECTOR n1{ XMLoadFloat3(&m.normals[refs[j]]) };
                    if (!isHardEdge)
                    {
                        for (u32 k{ j + 1 }; k < numRefs; ++k)
                        {
                            //This value represents the cosine of the angle between normals.
                            f32 cosTheta{ 0.f };
                            XMVECTOR n2{ XMLoadFloat3(&m.normals[refs[k]]) };
                            if (!isSoftEdge)
                            {
                                //NOTE
                                //We are accounting for the length of n1 in this calculation because
                                //it can possibly change in this loop iteration. We assume unit length
                                //for n2.
                                //cos(angle) = dot(n1, n2) / (||n1||*||n2||)
                                XMStoreFloat(&cosTheta, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
                            }

                            if (isSoftEdge || cosTheta >= cosAlpha)
                            {
                                n1 += n2;

                                m.indices[refs[k]] = m.indices[refs[j]];
                                refs.erase(refs.begin() + k);
                                --numRefs;
                                --k;
                            }
                        }
                    }

                    XMStoreFloat3(&v.normal, XMVector3Normalize(n1));
                }
            }
        }

        void processUvs(mesh& m)
        {
            utl::vector<vertex> oldVertices;
            oldVertices.swap(m.vertices);
            utl::vector<u32> oldIndices(m.indices.size());
            oldIndices.swap(m.indices);

            const u32 numVertices{ (u32)oldVertices.size() };
            const u32 numIndices{ (u32)oldIndices.size() };
            assert(numVertices && numIndices);

            utl::vector<utl::vector<u32>> idxRef(numVertices);

            for (u32 i{ 0 }; i < numIndices; ++i)
            {
                idxRef[oldIndices[i]].emplace_back(i);
            }

            for (u32 i{ 0 }; i < numIndices; ++i)
            {
                auto& refs{ idxRef[i] };
                u32 numRefs{ (u32)refs.size() };
                for (u32 j{ 0 }; j < numRefs; ++j)
                {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    vertex& v{ oldVertices[oldIndices[refs[j]]] };
                    v.uv = m.uvSets[0][refs[j]];
                    m.vertices.emplace_back(v);

                    for (u32 k{ j + 1 }; k < numRefs; ++k)
                    {
                        v2& uv1{ m.uvSets[0][refs[k]] };
                        if (XMScalarNearEqual(v.uv.x, uv1.x, epsilon) &&
                            XMScalarNearEqual(v.uv.y, uv1.y, epsilon))
                        {
                            m.indices[refs[k]] = m.indices[refs[j]];
                            refs.erase(refs.begin() + k);
                            --numRefs;
                            --k;
                        }
                    }
                }
            }
        }

        void packVerticesStatic(mesh& m)
        {
            const u32 numVertices{ (u32)m.vertices.size() };
            assert(numVertices);
            m.packedVerticesStatic.reserve(numVertices);

            for (u32 i{ 0 }; i < numVertices; ++i)
            {
                vertex& v{ m.vertices[i] };

                const u8 signs{ (u8)((v.normal.z > 0.f) << 1) };
                const u16 normalX{ (u16)packFloat<16>(v.normal.x, -1.f, 1.f) };
                const u16 normalY{ (u16)packFloat<16>(v.normal.y, -1.f, 1.f) };

                //TODO: Pack tangents in sign and in x/y components

                m.packedVerticesStatic.emplace_back(packedVertex::vertexStatic{ v.position, {0, 0, 0}, signs, {normalX, normalY}, {}, v.uv });
            }
		}

        void processVertices(mesh& m, const geometryImportSettings& settings)
        {
            assert((m.rawIndices.size() % 3) == 0);
            if (settings.calculateNormals || m.normals.empty())
            {
                recalculateNormals(m);
            }

            processNormals(m, settings.smoothingAngle);

            if (!m.uvSets.empty())
            {
                processUvs(m);
            }

            packVerticesStatic(m);
        }

        u64 getMeshSize(const mesh& m)
        {
            const u64 numVertices{ m.vertices.size() };
            const u64 vertexBufferSize{ sizeof(packedVertex::vertexStatic) * numVertices };
            const u64 indexSize{ (numVertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            const u64 indexBufferSize{ indexSize * m.indices.size() };
            constexpr u64 su32{ sizeof(u32) };
            const u64 size
            {
                su32 + m.name.size() + //Mesh name length and room for mesh name string
                su32 + //LOD ID
                su32 + //Vertex size
                su32 + //Number of vertices
                su32 + //Index size (16 bit or 32 bit)
                su32 + //number of indices
                sizeof(f32) + //LOD threshold
                vertexBufferSize + //Room for vertices
                indexBufferSize    //Room for indices
            };

            return size;
        }

        u64 getSceneSize(const scene& scene)
        {
            constexpr u64 su32{ sizeof(u32) };

            u64 size
            {
                su32 +               //Name length
                scene.name.size() +  //Room for scene name string
                su32                 //Number of LODs
            };

            for (auto& lod : scene.lodGroups)
            {
                u64 lodSize
                {
                    su32 + lod.name.size() + //LOD name length and room for LPD name string
                    su32                     //Number of meshes in this LOD
                };

                for (auto& m : lod.meshes)
                {
                    lodSize += getMeshSize(m);
                }

                size += lodSize;
            }

            return size;
        }

        void packMeshData(const mesh& m, u8* const buffer, u64& at)
        {
            constexpr u64 su32{ sizeof(u32) };
            u32 s{ 0 };

            //Mesh name
            s = (u32)m.name.size();
            memcpy(&buffer[at], &s, su32); at += su32;
            memcpy(&buffer[at], m.name.c_str(), s); at += s;

            //LOD ID
            s = m.lodId;
            memcpy(&buffer[at], &s, su32); at += su32;

            //Vertex size
            constexpr u32 vertexSize{ sizeof(packedVertex::vertexStatic) };
            s = vertexSize;
            memcpy(&buffer[at], &s, su32); at += su32;

            //Number of vertices
            const u32 numVertices{ (u32)m.vertices.size() };
            s = numVertices;
            memcpy(&buffer[at], &s, su32); at += su32;

            //Index size (16 bit or 32 bit)
            const u32 indexSize{ (numVertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            s = indexSize;
            memcpy(&buffer[at], &s, su32); at += su32;

            //Number of indices
            const u32 numIndices{ (u32)m.indices.size() };
            s = numIndices;
            memcpy(&buffer[at], &s, su32); at += su32;

            //LOD threshold
            memcpy(&buffer[at], &m.lodThreshold, sizeof(f32)); at += sizeof(f32);

            //Vertex data
            s = vertexSize * numVertices;
            memcpy(&buffer[at], m.packedVerticesStatic.data(), s); at += s;

            //Index data
            s = indexSize * numIndices;
            void* data{ (void*)m.indices.data() };
            utl::vector<u16> indices;
            if (indexSize == sizeof(u16))
            {
                indices.resize(numIndices);
                for (u32 i{ 0 }; i < numIndices; ++i) indices[i] = (u16)m.indices[i];
                data = (void*)indices.data();
            }
            memcpy(&buffer[at], data, s); at += s;
        }
	}

	void processScene(scene& scene, const geometryImportSettings& settings)
	{
        for (auto& lod : scene.lodGroups) 
        {
            for (auto& m : lod.meshes)
            {
                processVertices(m, settings);
            }
        }
	}

	void packData(const scene& scene, sceneData& data)
	{
        constexpr u64 su32{ sizeof(u32) };
		const u64 sceneSize{ getSceneSize(scene) };
		data.bufferSize = (u32)sceneSize;
        data.buffer = (u8*)CoTaskMemAlloc(sceneSize);

		assert(data.buffer);

        u8* const buffer{ data.buffer };
        u64 at{ 0 };
        u32 s{ 0 };

        //Scene name
        s = (u32)scene.name.size();
        memcpy(&buffer[at], &s, su32); at += su32;
        memcpy(&buffer[at], scene.name.c_str(), s); at += s;

        //Number of LODs
        s = (u32)scene.lodGroups.size();
        memcpy(&buffer[at], &s, su32); at += su32;

        for (auto& lod : scene.lodGroups)
        {
            //LOD name
            s = (u32)lod.name.size();
            memcpy(&buffer[at], &s, su32); at += su32;
            memcpy(&buffer[at], lod.name.c_str(), s); at += s;

            //Number of meshes in the LOD
            s = (u32)lod.meshes.size();
            memcpy(&buffer[at], &s, su32); at += su32;

            for (auto& m : lod.meshes)
            {
                packMeshData(m, buffer, at);
            }
        }
	}
}