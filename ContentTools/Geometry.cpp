#include "Geometry.h"
#include "Utilities/IOStream.h"

namespace mooncastle::tools
{
	namespace
	{
        using namespace math;
        using namespace DirectX;

        void recalculateNormals(mesh& m)
        {
            const u32 numIndices{ (u32)m.rawIndices.size() };
            m.normals.resize(numIndices);

            for (u32 i{ 0 }; i < numIndices; ++i)
            {
                const u32 i0{ m.rawIndices[i] };
                const u32 i1{ m.rawIndices[++i] };
                const u32 i2{ m.rawIndices[++i] };

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

            for (u32 i{ 0 }; i < numVertices; ++i)
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

        u64 getVertexElementSize(elements::elementTypes::type elementType)
        {
            using namespace elements;

            switch (elementType)
            {
            case elementTypes::staticNormal: return sizeof(staticNormal);
            case elementTypes::staticNormalTexture: return sizeof(staticNormalTexture);
            case elementTypes::staticColor: return sizeof(staticColor);
            case elementTypes::skeletal: return sizeof(skeletal);
            case elementTypes::skeletalColor: return sizeof(skeletalColor);
            case elementTypes::skeletalNormal: return sizeof(skeletalNormal);
            case elementTypes::skeletalNormalColor: return sizeof(skeletalNormalColor);
            case elementTypes::skeletalNormalTexture: return sizeof(skeletalNormalTexture);
            case elementTypes::skeletalNormalTextureColor: return sizeof(skeletalNormalTextureColor);
            }

            return 0;
        }

        void packVertices(mesh& m)
        {
            const u32 numVertices{ (u32)m.vertices.size() };

            assert(numVertices);

            m.positionBuffer.resize(sizeof(math::v3) * numVertices);
            math::v3* const positionBuffer{ (math::v3* const)m.positionBuffer.data() };

            for (u32 i{ 0 }; i < numVertices; ++i)
            {
                positionBuffer[i] = m.vertices[i].position;
            }

            struct u16v2 { u16 x, y; };
            struct u8v3 { u8 x, y, z; };

            utl::vector<u8>	tSigns(numVertices);
            utl::vector<u16v2> normals(numVertices);
            utl::vector<u16v2> tangents(numVertices);
            utl::vector<u8v3> jointWeights(numVertices);

            if (m.elementType & elements::elementTypes::staticNormal)
            {
                //Normals only.
                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    tSigns[i] = (u8)((v.normal.z > 0.0f) << 1);
                    normals[i] = { (u16)packFloat<16>(v.normal.x, -1.0f, 1.0f), (u16)packFloat<16>(v.normal.y, -1.0f, 1.0f) };
                }

                if (m.elementType & elements::elementTypes::staticNormalTexture)
                {
                    //Full T-space.
                    for (u32 i{ 0 }; i < numVertices; i++)
                    {
                        vertex& v{ m.vertices[i] };
                        tSigns[i] |= (u8)((v.tangent.w > 0.0f) && (v.tangent.z > 0.0f));
                        tangents[i] = { (u16)packFloat<16>(v.tangent.x, -1.0f, 1.0f), (u16)packFloat<16>(v.tangent.y, -1.0f, 1.0f) };
                    }
                }
            }

            if (m.elementType & elements::elementTypes::skeletal)
            {
                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };

                    //Pack joint weights (from [0.0, 1.0] to [0...255])
                    jointWeights[i] =
                    {
                        (u8)packUnitFloat<8>(v.jointWeights.x),
                        (u8)packUnitFloat<8>(v.jointWeights.y),
                        (u8)packUnitFloat<8>(v.jointWeights.z)
                    };
                    //w3 will be calculated in the shader since joint weights total to one(1).
                }
            }

            m.elementBuffer.resize(getVertexElementSize(m.elementType) * numVertices);

            using namespace elements;

            switch (m.elementType)
            {
            case elementTypes::staticColor:
            {
                staticColor* const elementBuffer{ (staticColor* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    elementBuffer[i] = { { v.red, v.green, v.blue }, {/*pad*/} };
                }
            }
            break;
            case elementTypes::staticNormal:
            {
                staticNormal* const elementBuffer{ (staticNormal* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    elementBuffer[i] = { { v.red, v.green, v.blue }, tSigns[i], {normals[i].x, normals[i].y} };
                }
            }
            break;
            case elementTypes::staticNormalTexture:
            {
                staticNormalTexture* const elementBuffer{ (staticNormalTexture* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    elementBuffer[i] = { { v.red, v.green, v.blue }, tSigns[i], {normals[i].x, normals[i].y}, {tangents[i].x, tangents[i].y}, v.uv };
                }
            }
            break;
            case elementTypes::skeletal:
            {
                skeletal* const element_buffer{ (skeletal* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.jointIndices.x, (u16)v.jointIndices.y, (u16)v.jointIndices.z, (u16)v.jointIndices.w };
                    element_buffer[i] = { {jointWeights[i].x, jointWeights[i].y, jointWeights[i].z}, {/*pad*/}, {indices[0], indices[1], indices[2], indices[3]} };
                }
            }
            break;
            case elementTypes::skeletalColor:
            {
                skeletalColor* const elementBuffer{ (skeletalColor* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.jointIndices.x, (u16)v.jointIndices.y, (u16)v.jointIndices.z, (u16)v.jointIndices.w };

                    elementBuffer[i] = { {jointWeights[i].x, jointWeights[i].y, jointWeights[i].z}, {/*pad*/},
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {v.red, v.green, v.blue}, {/*pad*/} };
                }
            }
            break;
            case elementTypes::skeletalNormal:
            {
                skeletalNormal* const elementBuffer{ (skeletalNormal* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.jointIndices.x, (u16)v.jointIndices.y, (u16)v.jointIndices.z, (u16)v.jointIndices.w };

                    elementBuffer[i] = { {jointWeights[i].x, jointWeights[i].y, jointWeights[i].z}, tSigns[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y} };
                }
            }
            break;
            case elementTypes::skeletalNormalColor:
            {
                skeletalNormalColor* const elementBuffer{ (skeletalNormalColor* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.jointIndices.x, (u16)v.jointIndices.y, (u16)v.jointIndices.z, (u16)v.jointIndices.w };

                    elementBuffer[i] = { {jointWeights[i].x, jointWeights[i].y, jointWeights[i].z}, tSigns[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y}, {v.red, v.green, v.blue}, {/*pad*/} };
                }
            }
            break;
            case elementTypes::skeletalNormalTexture:
            {
                skeletalNormalTexture* const elementBuffer{ (skeletalNormalTexture* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.jointIndices.x, (u16)v.jointIndices.y, (u16)v.jointIndices.z, (u16)v.jointIndices.w };

                    elementBuffer[i] = { {jointWeights[i].x, jointWeights[i].y, jointWeights[i].z}, tSigns[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y}, {tangents[i].x, tangents[i].y}, v.uv };
                }
            }
            break;
            case elementTypes::skeletalNormalTextureColor:
            {
                skeletalNormalTextureColor* const elementBuffer{ (skeletalNormalTextureColor* const)m.elementBuffer.data() };

                for (u32 i{ 0 }; i < numVertices; ++i)
                {
                    vertex& v{ m.vertices[i] };
                    const u16 indices[4]{ (u16)v.jointIndices.x, (u16)v.jointIndices.y, (u16)v.jointIndices.z, (u16)v.jointIndices.w };

                    elementBuffer[i] = { {jointWeights[i].x, jointWeights[i].y, jointWeights[i].z}, tSigns[i],
                                         {indices[0], indices[1], indices[2], indices[3]},
                                         {normals[i].x, normals[i].y}, {tangents[i].x, tangents[i].y}, v.uv,
                                         {v.red, v.green, v.blue}, {/*pad*/} };
                }
            }
            break;
            }
        }

        elements::elementTypes::type determineElementType(const mesh& m)
        {
            using namespace elements;

            elementTypes::type type{};

            if (m.normals.size())
            {
                if (m.uvSets.size() && m.uvSets[0].size())
                {
                    type = elementTypes::staticNormalTexture;
                }
                else
                {
                    type = elementTypes::staticNormal;
                }
            }
            else if (m.colors.size())
            {
                type = elementTypes::staticColor;
            }

            // TODO: Will expand for skeletal meshes.

            return type;
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

            m.elementType = determineElementType(m);
            packVertices(m);
        }

        u64 getMeshSize(const mesh& m)
        {
            const u64 numVertices{ m.vertices.size() };
            const u64 positionBufferSize{ m.positionBuffer.size() };
            assert(positionBufferSize == sizeof(math::v3) * numVertices);
            const u64 elementBufferSize{ m.elementBuffer.size() };
            assert(elementBufferSize == getVertexElementSize(m.elementType) * numVertices);
            const u64 indexSize{ (numVertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            const u64 indexBufferSize{ indexSize * m.indices.size() };
            constexpr u64 su32{ sizeof(u32) };
            const u64 size
            {
                su32 + m.name.size() +  //Mesh name length and room for mesh name string
                su32 +                  //LOD ID
				su32 +                  //Vertex element size (excluding position)
                su32 +                  //Element type
                su32 +                  //Number of vertices
                su32 +                  //Index size (16 bit or 32 bit)
                su32 +                  //number of indices
                sizeof(f32) +           //LOD threshold
                positionBufferSize +    //Room for vertex positions
				elementBufferSize +     //Room for vertex elements
                indexBufferSize         //Room for indices
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

            for (const auto& lod : scene.lodGroups)
            {
                u64 lodSize
                {
                    su32 + lod.name.size() + //LOD name length and room for LPD name string
                    su32                     //Number of meshes in this LOD
                };

                for (const auto& m : lod.meshes)
                {
                    lodSize += getMeshSize(m);
                }

                size += lodSize;
            }

            return size;
        }

        void packMeshData(const mesh& m, utl::blobStreamWriter& blob)
        {
            //Mesh name.
            blob.write((u32)m.name.size());
            blob.write(m.name.c_str(), m.name.size());

            //LOD ID.
            blob.write(m.lodId);

            //Vertex element size.
            const u32 elementsSize{ (u32)getVertexElementSize(m.elementType) };
            blob.write(elementsSize);

            //Elements type.
            blob.write((u32)m.elementType);

            //Number of vertices.
            const u32 numVertices{ (u32)m.vertices.size() };
            blob.write(numVertices);

            //Index size.
            const u32 indexSize{ (numVertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            blob.write(indexSize);

            //Number of indices.
            const u32 numIndices{ (u32)m.indices.size() };
            blob.write(numIndices);

            //LOD threshold.
            blob.write(m.lodThreshold);

            //Position buffer.
            assert(m.positionBuffer.size() == sizeof(math::v3) * numVertices);
            blob.write(m.positionBuffer.data(), m.positionBuffer.size());

            //Element buffer.
            assert(m.elementBuffer.size() == elementsSize * numVertices);
            blob.write(m.elementBuffer.data(), m.elementBuffer.size());

            //Index data.
            const u32 indexBufferSize{ indexSize * numIndices };
            const u8* data{ (const u8*)m.indices.data() };
            utl::vector<u16> indices;

            if (indexSize == sizeof(u16))
            {
                indices.resize(numIndices);

                for (u32 i{ 0 }; i < numIndices; ++i)
                {
                    indices[i] = (u16)m.indices[i];
                    data = (const u8*)indices.data();
                }
            }

            blob.write(data, indexBufferSize);
        }

        bool splitMeshesByMaterial(u32 materialIndex, const mesh& m, mesh& submesh)
        {
            submesh.name = m.name;
            submesh.lodThreshold = m.lodThreshold;
            submesh.lodId = m.lodId;
            submesh.materialUsed.emplace_back(materialIndex);
            submesh.uvSets.resize(m.uvSets.size());

            const u32 numPolys{ (u32)m.rawIndices.size() / 3 };
            utl::vector<u32> vertexRef(m.positions.size(), u32_invalid_id);

            for (u32 i{ 0 }; i < numPolys; ++i)
            {
                const u32 mtlIndex{ m.materialIndices[i] };

                if (mtlIndex != materialIndex) continue;

                const u32 index{ i * 3 };

                for (u32 j = index; j < index + 3; ++j)
                {
                    const u32 vIdx{ m.rawIndices[j] };

                    if (vertexRef[vIdx] != u32_invalid_id)
                    {
                        submesh.rawIndices.emplace_back(vertexRef[vIdx]);
                    }
                    else
                    {
                        submesh.rawIndices.emplace_back((u32)submesh.positions.size());
                        vertexRef[vIdx] = submesh.rawIndices.back();
                        submesh.positions.emplace_back(m.positions[vIdx]);
                    }
                    
                    if (m.normals.size()) submesh.normals.emplace_back(m.normals[j]); 
                    if (m.tangents.size()) submesh.tangents.emplace_back(m.tangents[j]);

                    for (u32 k{ 0 }; k < m.uvSets.size(); ++k)
                    {
                        if (m.uvSets[k].size())
                        {
                            submesh.uvSets[k].emplace_back(m.uvSets[k][j]);
                        }
                    }
                }
            }

            assert((submesh.rawIndices.size() % 3) == 0);

            return !submesh.rawIndices.empty();
        }

        void splitMeshesByMaterial(scene& scene, progression *const progression)
        {
            assert(progression);

            progression->setCallback(0, 0);

            for (auto& lod : scene.lodGroups)
            {
                utl::vector<mesh> newMeshes;

                for (const auto& m : lod.meshes)
                {
                    //If more than one material is used in this mesh, then split it into submeshes.
                    const u32 numMaterials{ (u32)m.materialUsed.size() };

                    if (numMaterials > 1)
                    {
                        for (u32 i{ 0 }; i < numMaterials; ++i)
                        {
                            mesh submesh{};

                            if (splitMeshesByMaterial(m.materialUsed[i], m, submesh))
                            {
                                newMeshes.emplace_back(submesh);
                            }
                        }
                    }
                    else
                    {
                        newMeshes.emplace_back(m);
                    }
                }

                progression->setCallback(progression->getValue(), progression->getMaxValue() + (u32)newMeshes.size());
                newMeshes.swap(lod.meshes);
            }
        }

        template <typename T> void appendToVectorPod(utl::vector<T>& destination, const utl::vector<T>& source)
        {
            if (source.empty()) return;

            const u32 numElements{ (u32)destination.size() };
            destination.resize(destination.size() + source.size());
            memcpy(&destination[numElements], source.data(), source.size() * sizeof(T));
        }
	}

	void processScene(scene& scene, const geometryImportSettings& settings, progression *const progression)
	{
        assert(progression);

		splitMeshesByMaterial(scene, progression);

        for (auto& lod : scene.lodGroups) 
        {
            for (auto& m : lod.meshes)
            {
                processVertices(m, settings);
                progression->setCallback(progression->getValue() + 1, progression->getMaxValue());
            }
        }
	}

	void packData(const scene& scene, sceneData& data)
	{
        const u64 sceneSize{ getSceneSize(scene) };
        data.bufferSize = (u32)sceneSize;
        data.buffer = (u8*)CoTaskMemAlloc(sceneSize);

        assert(data.buffer);

        utl::blobStreamWriter blob{ data.buffer, data.bufferSize };

        //Scene name.
        blob.write((u32)scene.name.size());
        blob.write(scene.name.c_str(), scene.name.size());

        //Number of LODs.
        blob.write((u32)scene.lodGroups.size());

        for (const auto& lod : scene.lodGroups)
        {
            //LOD name.
            blob.write((u32)lod.name.size());
            blob.write(lod.name.c_str(), lod.name.size());

            //Number of meshes in this LOD.
            blob.write((u32)lod.meshes.size());

            for (const auto& m : lod.meshes)
            {
                packMeshData(m, blob);
            }
        }

        assert(sceneSize == blob.getOffset());
	}

    bool coalesceMeshes(const lodGroup& lod, mesh& combinedMesh, progression *const progression)
    {
        assert(lod.meshes.size());

        const mesh& firstMesh{ lod.meshes[0] };
        combinedMesh.name = firstMesh.name;
        combinedMesh.elementType = determineElementType(firstMesh);
        combinedMesh.lodThreshold = firstMesh.lodThreshold;
        combinedMesh.lodId = firstMesh.lodId;
        combinedMesh.uvSets.resize(firstMesh.uvSets.size());

        for (u32 meshIndex{ 0 }; meshIndex < lod.meshes.size(); ++meshIndex)
        {
            const mesh& m{ lod.meshes[meshIndex] };

            if (combinedMesh.elementType != determineElementType(m) ||
                combinedMesh.uvSets.size() != m.uvSets.size() ||
                combinedMesh.lodId != m.lodId ||
                !math::isEqual(combinedMesh.lodThreshold, m.lodThreshold))
            {
                combinedMesh = {};

                return false;
            }
        }

        for (u32 meshIndex{ 0 }; meshIndex < lod.meshes.size(); ++meshIndex)
        {
            const mesh& m{ lod.meshes[meshIndex] };
            const u32 posCount{ (u32)combinedMesh.positions.size() };
            const u32 rawIndexBase{ (u32)combinedMesh.rawIndices.size() };

            appendToVectorPod(combinedMesh.positions, m.positions);
            appendToVectorPod(combinedMesh.normals, m.normals);
            appendToVectorPod(combinedMesh.tangents, m.tangents);
            appendToVectorPod(combinedMesh.colors, m.colors);

            for (u32 i{ 0 }; i < combinedMesh.uvSets.size(); ++i)
            {
                appendToVectorPod(combinedMesh.uvSets[i], m.uvSets[i]);
            }

            appendToVectorPod(combinedMesh.materialIndices, m.materialIndices);
            appendToVectorPod(combinedMesh.rawIndices, m.rawIndices);

            for (u32 i{ rawIndexBase }; i < combinedMesh.rawIndices.size(); ++i)
            {
                combinedMesh.rawIndices[i] += posCount;
            }

            progression->setCallback(progression->getValue(), progression->getMaxValue() > 1 ? progression->getMaxValue() - 1 : 1);
        }

        for (const u32 materialIndex : combinedMesh.materialIndices)
        {
            if (std::find(combinedMesh.materialUsed.begin(), combinedMesh.materialUsed.end(), materialIndex) == combinedMesh.materialUsed.end())
            {
                combinedMesh.materialUsed.emplace_back(materialIndex);
            }
        }

        return true;
    }
}