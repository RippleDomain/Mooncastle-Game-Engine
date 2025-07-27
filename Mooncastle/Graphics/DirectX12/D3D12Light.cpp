#include "D3D12Light.h"
#include "Shaders/SharedTypes.h"
#include "D3D12Core.h"
#include "EngineAPI/GameEntity.h"
#include "Components/Transform.h"

namespace mooncastle::graphics::d3D12::light 
{
    namespace 
    {
        template<u32 n>
        struct u32SetBits 
        {
            static_assert(n > 0 && n <= 32);
            constexpr static const u32 bits{ u32SetBits<n - 1>::bits | (1 << (n - 1)) };
        };

        template<>
        struct u32SetBits<0> 
        {
            constexpr static const u32 bits{ 0 };
        };

        static_assert(u32SetBits<frameBufferCount>::bits < (1 << 8), "That's quite a large frame buffer count!");

        constexpr u8 dirtyBitsMask{ (u8)u32SetBits<frameBufferCount>::bits };

        struct lightOwner
        {
            gameEntity::entityId    entityID{ id::invalidId };
            u32                     dataIndex{ u32_invalid_id };
            graphics::light::type   type;
            bool                    isEnabled;
        };

#if USE_STL_VECTOR
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

        class lightSet
        {
        public:
            constexpr graphics::light add(const lightInitInfo& info)
            {
                if (info.type == graphics::light::directional)
                {
                    u32 index{ u32_invalid_id };

                    for (u32 i{ 0 }; i < nonCullableOwners.size(); ++i)
                    {
                        if (!id::isValid(nonCullableOwners[i]))
                        {
                            index = i;
                            break;
                        }
                    }

                    if (index == u32_invalid_id)
                    {
                        index = (u32)nonCullableOwners.size();

                        nonCullableOwners.emplace_back();
                        nonCullableLights.emplace_back();
                    }

                    hlsl::DirectionalLightParameters& params{ nonCullableLights[index] };
                    params.Color = info.color;
                    params.Intensity = info.intensity;

                    lightOwner owner{ gameEntity::entityId{info.entityID}, index, info.type, info.isEnabled };
                    const lightId id{ owners.add(owner) };
                    nonCullableOwners[index] = id;

                    return graphics::light{ id, info.lightSetKey };
                }
                else
                {
                    u32 index{ u32_invalid_id };

                    // Try to find an empty slot
                    for (u32 i{ enabledLightCount }; i < cullableOwners.size(); ++i)
                    {
                        if (!id::isValid(cullableOwners[i]))
                        {
                            index = i;
                            break;
                        }
                    }

                    if (index == u32_invalid_id)
                    {
                        index = (u32)cullableOwners.size();
                        cullableLights.emplace_back();
                        cullingInfo.emplace_back();
                        cullableEntityIDs.emplace_back();
                        cullableOwners.emplace_back();
                        dirtyBits.emplace_back();

                        assert(cullableOwners.size() == cullableLights.size());
                        assert(cullableOwners.size() == cullingInfo.size());
                        assert(cullableOwners.size() == cullableEntityIDs.size());
                        assert(cullableOwners.size() == dirtyBits.size());
                    }

                    addCullableLightParams(info, index);
                    addLightCullingInfo(info, index);

                    const lightId id{ owners.add(lightOwner{gameEntity::entityId{info.entityID}, index, info.type, info.isEnabled}) };
                    cullableEntityIDs[index] = owners[id].entityID;
                    cullableOwners[index] = id;
                    dirtyBits[index] = dirtyBitsMask;
                    setEnabled(id, info.isEnabled);
                    updateTransform(index);

                    return graphics::light{ id, info.lightSetKey };
                }
            }

            constexpr void remove(lightId id)
            {
                setEnabled(id, false);

                const lightOwner& owner{ owners[id] };

                if (owner.type == graphics::light::directional)
                {
                    nonCullableOwners[owner.dataIndex] = lightId{ id::invalidId };
                }
                else
                {
                    //Cullable lights.
                    assert(owners[cullableOwners[owner.dataIndex]].dataIndex == owner.dataIndex);
                    cullableOwners[owner.dataIndex] = lightId{ id::invalidId };
                }

                owners.remove(id);
            }

            void updateTransforms()
            {
                //Updates the directions for non-cullable lights.
                for (const auto& id : nonCullableOwners)
                {
                    if (!id::isValid(id)) continue;

                    const lightOwner& owner{ owners[id] };
                    if (owner.isEnabled)
                    {
                        const gameEntity::entity entity{ gameEntity::entityId(owner.entityID) };
                        hlsl::DirectionalLightParameters& params{ nonCullableLights[owner.dataIndex] };
                        params.Direction = entity.orientation();
                    }
                }

                //Cullable lights.
                const u32 count{ enabledLightCount };
                if (!count) return;
                assert(cullableEntityIDs.size() >= count);

                transformFlagsCache.resize(count);
                transform::getUpdatedComponentFlags(cullableEntityIDs.data(), count, transformFlagsCache.data());

                for (u32 i{ 0 }; i < count; ++i)
                {
                    if (transformFlagsCache[i])
                    {
                        updateTransform(i);
                    }
                }
            }

            constexpr void setEnabled(lightId id, bool isEnabled)
            {
                owners[id].isEnabled = isEnabled;

                if (owners[id].type == graphics::light::directional)
                {
                    return;
                }

                //Cullable lights.
                const u32 dataIndex{ owners[id].dataIndex };
                u32& count{ enabledLightCount };

                if (isEnabled)
                {
                    if (dataIndex > count)
                    {
                        assert(count < cullableLights.size());
                        swapCullableLights(dataIndex, count);
                        ++count;
                    }
                    else if (dataIndex == count)
                    {
                        ++count;
                    }
                }
                else if (count > 0)
                {
                    const u32 last{ count - 1 };
                    if (dataIndex < last)
                    {
                        swapCullableLights(dataIndex, last);
                        --count;
                    }
                    else if (dataIndex == last)
                    {
                        --count;
                    }
                }
            }

            constexpr void setIntensity(lightId id, f32 intensity)
            {
                if (intensity < 0.f) intensity = 0.f;

                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                if (owner.type == graphics::light::directional)
                {
                    assert(index < nonCullableLights.size());
                    nonCullableLights[index].Intensity = intensity;
                }
                else
                {
                    assert(owners[cullableOwners[index]].dataIndex == index);
                    assert(index < cullableLights.size());
                    cullableLights[index].Intensity = intensity;
                    dirtyBits[index] = dirtyBitsMask;
                }
            }

            constexpr void setColor(lightId id, math::v3 color)
            {
                assert(color.x <= 1.f && color.y <= 1.f && color.z <= 1.f);
                assert(color.x >= 0.f && color.y >= 0.f && color.z >= 0.f);

                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                if (owner.type == graphics::light::directional)
                {
                    assert(index < nonCullableLights.size());
                    nonCullableLights[index].Color = color;
                }
                else
                {
                    assert(owners[cullableOwners[index]].dataIndex == index);
                    assert(index < cullableLights.size());
                    cullableLights[index].Color = color;
                    dirtyBits[index] = dirtyBitsMask;
                }
            }

            CONSTEXPR void setAttenuation(lightId id, math::v3 attenuation)
            {
                assert(attenuation.x >= 0.f && attenuation.y >= 0.f && attenuation.z >= 0.f);

                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type != graphics::light::directional);
                assert(index < cullableLights.size());

                cullableLights[index].Attenuation = attenuation;
                dirtyBits[index] = dirtyBitsMask;
            }

            CONSTEXPR void setRange(lightId id, f32 range)
            {
                assert(range > 0.f);

                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type != graphics::light::directional);
                assert(index < cullableLights.size());

                cullableLights[index].Range = range;
                cullingInfo[index].Range = range;
                dirtyBits[index] = dirtyBitsMask;

                if (owner.type == graphics::light::spot)
                {
                    cullingInfo[index].ConeRadius = calculateConeRadius(range, cullableLights[index].CosPenumbra);
                }
            }

            void setUmbra(lightId id, f32 umbra)
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type == graphics::light::spot);
                assert(index < cullableLights.size());

                umbra = math::clamp(umbra, 0.f, math::pi);
                cullableLights[index].CosUmbra = DirectX::XMScalarCos(umbra * 0.5f);
                dirtyBits[index] = dirtyBitsMask;

                if (getPenumbra(id) < umbra)
                {
                    setPenumbra(id, umbra);
                }
            }

            void setPenumbra(lightId id, f32 penumbra)
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type == graphics::light::spot);
                assert(index < cullableLights.size());

                penumbra = math::clamp(penumbra, getUmbra(id), math::pi);
                cullableLights[index].CosPenumbra = DirectX::XMScalarCos(penumbra * 0.5f);

                cullingInfo[index].ConeRadius = calculateConeRadius(getRange(id), cullableLights[index].CosPenumbra);
                dirtyBits[index] = dirtyBitsMask;
            }

            constexpr bool isEnabled(lightId id) const
            {
                return owners[id].isEnabled;
            }

            constexpr f32 getIntensity(lightId id) const
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                if (owner.type == graphics::light::directional)
                {
                    assert(index < nonCullableLights.size());
                    return nonCullableLights[index].Intensity;
                }
                
                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(index < cullableLights.size());

                return cullableLights[index].Intensity;
            }

            constexpr math::v3 getColor(lightId id) const
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                if (owner.type == graphics::light::directional)
                {
                    assert(index < nonCullableLights.size());
                    return nonCullableLights[index].Color;
                }
                
                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(index < cullableLights.size());

                return cullableLights[index].Color;
            }

            CONSTEXPR math::v3 getAttenuation(lightId id) const
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type != graphics::light::directional);
                assert(index < cullableLights.size());

                return cullableLights[index].Attenuation;
            }

            CONSTEXPR f32 getRange(lightId id) const
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type != graphics::light::directional);
                assert(index < cullableLights.size());

                return cullableLights[index].Range;
            }

            f32 getUmbra(lightId id) const
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type == graphics::light::spot);
                assert(index < cullableLights.size());

                return DirectX::XMScalarACos(cullableLights[index].CosUmbra) * 2.f;
            }

            f32 getPenumbra(lightId id) const
            {
                const lightOwner& owner{ owners[id] };
                const u32 index{ owner.dataIndex };

                assert(owners[cullableOwners[index]].dataIndex == index);
                assert(owner.type == graphics::light::spot);
                assert(index < cullableLights.size());

                return DirectX::XMScalarACos(cullableLights[index].CosPenumbra) * 2.f;
            }

            constexpr graphics::light::type getType(lightId id) const
            {
                return owners[id].type;
            }

            constexpr id::idType getEntityID(lightId id) const
            {
                return owners[id].entityID;
            }

            //Returns the number of enabled directional lights.
            CONSTEXPR u32 getNonCullableLightCount() const
            {
                u32 count{ 0 };

                for (const auto& id : nonCullableOwners)
                {
                    if (id::isValid(id) && owners[id].isEnabled) ++count;
                }
                return count;
            }

            CONSTEXPR void setNonCullableLights(hlsl::DirectionalLightParameters* const lights, [[maybe_unused]] u32 bufferSize)
            {
                assert(bufferSize == math::alignSizeUp<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(getNonCullableLightCount() * sizeof(hlsl::DirectionalLightParameters)));

                const u32 count{ (u32)nonCullableOwners.size() };

                u32 index{ 0 };

                for (u32 i{ 0 }; i < count; ++i)
                {
					//Since the containers are not tightly packed, skips over the indexes that contain invalid values.
                    if (!id::isValid(nonCullableOwners[i])) continue;

                    const lightOwner& owner{ owners[nonCullableOwners[i]] };
                    if (owner.isEnabled)
                    {
                        assert(owners[nonCullableOwners[i]].dataIndex == i);

                        lights[index] = nonCullableLights[i];
                        ++index;
                    }
                }
            }

            constexpr u32 getCullableLightCount() const
            {
                return enabledLightCount;
            }

            constexpr bool hasLights() const
            {
                return owners.getSize() > 0;
            }

        private:
            f32 calculateConeRadius(f32 range, f32 cosPenumbra)
            {
                const f32 sinPenumbra{ sqrt(1.f - cosPenumbra * cosPenumbra) };
                return sinPenumbra * range;
            }

            void updateTransform(u32 index)
            {
                const gameEntity::entity entity{ gameEntity::entityId{cullableEntityIDs[index]} };
                hlsl::LightParameters& params{ cullableLights[index] };
                params.Position = entity.position();

                hlsl::LightCullingLightInfo& newCullingInfo{ cullingInfo[index] };
                newCullingInfo.Position = params.Position;

                if (params.Type == graphics::light::spot)
                {
                    newCullingInfo.Direction = params.Direction = entity.orientation();
                }

                dirtyBits[index] = dirtyBitsMask;
            }

            CONSTEXPR void addCullableLightParams(const lightInitInfo& info, u32 index)
            {
                using graphics::light;
                assert(info.type != light::directional && index < cullableLights.size());

                hlsl::LightParameters& params{ cullableLights[index] };
                params.Type = info.type;
                assert(params.Type < light::count);
                params.Color = info.color;
                params.Intensity = info.intensity;

                if (params.Type == light::point)
                {
                    const pointLightParameters& p{ info.pointParams };

                    params.Attenuation = p.attenuation;
                    params.Range = p.range;
                }
                else if (params.Type == light::spot)
                {
                    const spotLightParameters& p{ info.spotParams };

                    params.Attenuation = p.attenuation;
                    params.Range = p.range;
                    params.CosUmbra = DirectX::XMScalarCos(p.umbra * 0.5f);
                    params.CosPenumbra = DirectX::XMScalarCos(p.penumbra * 0.5f);
                }
            }

            CONSTEXPR void addLightCullingInfo(const lightInitInfo& info, u32 index)
            {
                using graphics::light;
                assert(info.type != light::directional && index < cullingInfo.size());

                hlsl::LightParameters& params{ cullableLights[index] };
                assert(params.Type == info.type);

                hlsl::LightCullingLightInfo& newCullingInfo{ cullingInfo[index] };
                newCullingInfo.Range = params.Range;

                newCullingInfo.Type = params.Type;

                if (info.type == light::spot)
                {
                    newCullingInfo.ConeRadius = calculateConeRadius(params.Range, params.CosPenumbra);
                }
            }

            void swapCullableLights(u32 index1, u32 index2)
            {
                assert(index1 != index2);
                assert(index1 < cullableOwners.size());
                assert(index2 < cullableOwners.size());
                assert(index1 < cullableLights.size());
                assert(index2 < cullableLights.size());
                assert(index1 < cullingInfo.size());
                assert(index2 < cullingInfo.size());
                assert(index1 < cullableEntityIDs.size());
                assert(index2 < cullableEntityIDs.size());
                assert(id::isValid(cullableOwners[index1]) || id::isValid(cullableOwners[index2]));

                if (!id::isValid(cullableOwners[index2]))
                {
                    std::swap(index1, index2);
                }

                if (!id::isValid(cullableOwners[index1]))
                {
                    lightOwner& owner2{ owners[cullableOwners[index2]] };

                    assert(owner2.dataIndex == index2);

                    owner2.dataIndex = index1;

                    cullableLights[index1] = cullableLights[index2];
                    cullingInfo[index1] = cullingInfo[index2];
                    cullableEntityIDs[index1] = cullableEntityIDs[index2];
                    std::swap(cullableOwners[index1], cullableOwners[index2]);
                    dirtyBits[index1] = dirtyBitsMask;

                    assert(owners[cullableOwners[index1]].entityID == cullableEntityIDs[index1]);
                    assert(id::isValid(cullableOwners[index2]));
                }
                else
                {
                    lightOwner& owner1{ owners[cullableOwners[index1]] };
                    lightOwner& owner2{ owners[cullableOwners[index2]] };

                    assert(owner1.dataIndex == index1);
                    assert(owner2.dataIndex == index2);

                    owner1.dataIndex = index2;
                    owner2.dataIndex = index1;

                    std::swap(cullableLights[index1], cullableLights[index2]);
                    std::swap(cullingInfo[index1], cullingInfo[index2]);
                    std::swap(cullableEntityIDs[index1], cullableEntityIDs[index2]);
                    std::swap(cullableOwners[index1], cullableOwners[index2]);

                    assert(owners[cullableOwners[index1]].entityID == cullableEntityIDs[index1]);
                    assert(owners[cullableOwners[index2]].entityID == cullableEntityIDs[index2]);
                    assert(index1 < dirtyBits.size());
                    assert(index2 < dirtyBits.size());

                    dirtyBits[index1] = dirtyBitsMask;
                    dirtyBits[index2] = dirtyBitsMask;
                }
            }

            //These containers are NOT tightly packed.
            utl::freeList<lightOwner>                       owners;
            utl::vector<hlsl::DirectionalLightParameters>   nonCullableLights;
            utl::vector<lightId>                            nonCullableOwners;

            //These containers are tightly packed.
            utl::vector<hlsl::LightParameters>              cullableLights;
            utl::vector<hlsl::LightCullingLightInfo>        cullingInfo;
            utl::vector<gameEntity::entityId>               cullableEntityIDs;
            utl::vector<lightId>                            cullableOwners;
            utl::vector<u8>                                 dirtyBits;

			utl::vector<u8>								    transformFlagsCache;    
            u32                                             enabledLightCount{ 0 };

            friend class                                    D3D12LightBuffer;
        };

        class D3D12LightBuffer
        {
        public:
            D3D12LightBuffer() = default;

            CONSTEXPR void updateLightBuffers(lightSet& set, u64 lightSetKey, u32 frameIndex)
            {
                u32 sizes[lightBuffer::count]{};
                sizes[lightBuffer::nonCullableLight] = set.getNonCullableLightCount() * sizeof(hlsl::DirectionalLightParameters);
                sizes[lightBuffer::cullableLight] = set.getCullableLightCount() * sizeof(hlsl::LightParameters);
                sizes[lightBuffer::cullingInfo] = set.getCullableLightCount() * sizeof(hlsl::LightCullingLightInfo);

                u32 currentSizes[lightBuffer::count]{};
                currentSizes[lightBuffer::nonCullableLight] = buffers[lightBuffer::nonCullableLight].buffer.getSize();
                currentSizes[lightBuffer::cullableLight] = buffers[lightBuffer::cullableLight].buffer.getSize();
                currentSizes[lightBuffer::cullingInfo] = buffers[lightBuffer::cullingInfo].buffer.getSize();

                if (currentSizes[lightBuffer::nonCullableLight] < sizes[lightBuffer::nonCullableLight])
                {
                    resizeBuffer(lightBuffer::nonCullableLight, sizes[lightBuffer::nonCullableLight], frameIndex);
                }

                set.setNonCullableLights((hlsl::DirectionalLightParameters* const)buffers[lightBuffer::nonCullableLight].cpuAddress,
                    buffers[lightBuffer::nonCullableLight].buffer.getSize());

                //Updates cullable light buffers.
                bool buffersResized{ false };

                if (currentSizes[lightBuffer::cullableLight] < sizes[lightBuffer::cullableLight])
                {
                    assert(currentSizes[lightBuffer::cullingInfo] < sizes[lightBuffer::cullingInfo]);

                    resizeBuffer(lightBuffer::cullableLight, sizes[lightBuffer::cullableLight], frameIndex);
                    resizeBuffer(lightBuffer::cullingInfo, sizes[lightBuffer::cullingInfo], frameIndex);
                    buffersResized = true;
                }

                bool allLightsUpdated{ false };

                if (buffersResized || currentLightSetKey != lightSetKey)
                {
                    memcpy(buffers[lightBuffer::cullableLight].cpuAddress, set.cullableLights.data(), sizes[lightBuffer::cullableLight]);
                    memcpy(buffers[lightBuffer::cullingInfo].cpuAddress, set.cullingInfo.data(), sizes[lightBuffer::cullingInfo]);
                    currentLightSetKey = lightSetKey;
                    allLightsUpdated = true;
                }

                assert(currentLightSetKey == lightSetKey);

                const u32 indexMask{ 1UL << frameIndex };

                if (allLightsUpdated)
                {
                    for (u32 i{ 0 }; i < set.getCullableLightCount(); ++i)
                    {
                        set.dirtyBits[i] &= ~indexMask;
                    }
                }
                else
                {
                    for (u32 i{ 0 }; i < set.getCullableLightCount(); ++i)
                    {
                        if (set.dirtyBits[i] & indexMask)
                        {
                            assert(i * sizeof(hlsl::LightParameters) < sizes[lightBuffer::cullableLight]);
                            assert(i * sizeof(hlsl::LightCullingLightInfo) < sizes[lightBuffer::cullingInfo]);

                            u8* const lightDst = buffers[lightBuffer::cullableLight].cpuAddress + (i * sizeof(hlsl::LightParameters));
                            u8* const cullingDst = buffers[lightBuffer::cullingInfo].cpuAddress + (i * sizeof(hlsl::LightCullingLightInfo));
                            memcpy(lightDst, &set.cullableLights[i], sizeof(hlsl::LightParameters));
                            memcpy(cullingDst, &set.cullingInfo[i], sizeof(hlsl::LightCullingLightInfo));

                            set.dirtyBits[i] &= ~indexMask;
                        }
                    }
                }
            }

            constexpr void release()
            {
                for (u32 i{ 0 }; i < lightBuffer::count; ++i)
                {
                    buffers[i].buffer.release();
                    buffers[i].cpuAddress = nullptr;
                }
            }

            constexpr D3D12_GPU_VIRTUAL_ADDRESS getNonCullableLights() const 
            { 
                return buffers[lightBuffer::nonCullableLight].buffer.getGPUAddress(); 
            }
            constexpr D3D12_GPU_VIRTUAL_ADDRESS getCullableLights() const 
            { 
                return buffers[lightBuffer::cullableLight].buffer.getGPUAddress(); 
            }
            constexpr D3D12_GPU_VIRTUAL_ADDRESS getCullingInfo() const 
            { 
                return buffers[lightBuffer::cullingInfo].buffer.getGPUAddress(); 
            }

        private:
            struct lightBuffer
            {
                enum type : u32
                {
                    nonCullableLight,
                    cullableLight,
                    cullingInfo,
                    count
                };

                D3D12Buffer     buffer{};
                u8*             cpuAddress{ nullptr };
            };

            void resizeBuffer(lightBuffer::type type, u32 size, [[maybe_unused]] u32 frameIndex)
            {
                assert(type < lightBuffer::count);
                if (!size) return;

                buffers[type].buffer.release();
                buffers[type].buffer = D3D12Buffer{ constantBuffer::getDefaultInitInfo(size), true };

                NAME_D3D12_OBJECT_INDEXED(buffers[type].buffer.getBuffer(), frameIndex,
                    type == lightBuffer::nonCullableLight ? L"Non-cullable Light Buffer" :
                    type == lightBuffer::cullableLight ? L"Cullable Light Buffer" : L"Light Culling Info Buffer");

                D3D12_RANGE range{};

                DXCall(buffers[type].buffer.getBuffer()->Map(0, &range, (void**)&buffers[type].cpuAddress));
                assert(buffers[type].cpuAddress);
            }

            lightBuffer     buffers[lightBuffer::count];
            u64             currentLightSetKey{ 0 };
        };

		std::unordered_map<u64, lightSet> lightSets;
		D3D12LightBuffer                  lightBuffers[frameBufferCount];

        constexpr void setIsEnabled(lightSet& set, lightId id, const void* const data, [[maybe_unused]] u32 size)
        {
            bool isEnabled{ *(bool*)data };
            assert(sizeof(isEnabled) == size);
            set.setEnabled(id, isEnabled);
        }

        constexpr void setIntensity(lightSet& set, lightId id, const void* const data, [[maybe_unused]] u32 size)
        {
            f32 intensity{ *(f32*)data };
            assert(sizeof(intensity) == size);
            set.setIntensity(id, intensity);
        }

        constexpr void setColor(lightSet& set, lightId id, const void* const data, [[maybe_unused]] u32 size)
        {
            math::v3 color{ *(math::v3*)data };
            assert(sizeof(color) == size);
            set.setColor(id, color);
        }

        CONSTEXPR void setAttenuation(lightSet& set, lightId id, const void* const data, [[maybe_unused]] u32 size)
        {
            math::v3 attenuation{ *(math::v3*)data };
            assert(sizeof(attenuation) == size);
            set.setAttenuation(id, attenuation);
        }

        CONSTEXPR void setRange(lightSet& set, lightId id, const void* const data, [[maybe_unused]] u32 size)
        {
            f32 range{ *(f32*)data };
            assert(sizeof(range) == size);
            set.setRange(id, range);
        }

        void setUmbra(lightSet& set, lightId id, const void* const data, [[maybe_unused]] u32 size)
        {
            f32 umbra{ *(f32*)data };
            assert(sizeof(umbra) == size);
            set.setUmbra(id, umbra);
        }

        void setPenumbra(lightSet& set, lightId id, const void* const data, [[maybe_unused]] u32 size)
        {
            f32 penumbra{ *(f32*)data };
            assert(sizeof(penumbra) == size);
            set.setPenumbra(id, penumbra);
        }

        constexpr void getIsEnabled(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            bool* const isEnabled{ (bool* const)data };
            assert(sizeof(bool) == size);
            *isEnabled = set.isEnabled(id);
        }

        constexpr void getIntensity(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            f32* const intensity{ (f32* const)data };
            assert(sizeof(f32) == size);
            *intensity = set.getIntensity(id);
        }

        constexpr void getColor(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            math::v3* const color{ (math::v3* const)data };
            assert(sizeof(math::v3) == size);
            *color = set.getColor(id);
        }

        CONSTEXPR void getAttenuation(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            math::v3* const attenuation{ (math::v3* const)data };
            assert(sizeof(math::v3) == size);
            *attenuation = set.getAttenuation(id);
        }

        constexpr void getRange(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            f32* const range{ (f32* const)data };
            assert(sizeof(f32) == size);
            *range = set.getRange(id);
        }

        void getUmbra(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            f32* const umbra{ (f32* const)data };
            assert(sizeof(f32) == size);
            *umbra = set.getUmbra(id);
        }

        void getPenumbra(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            f32* const penumbra{ (f32* const)data };
            assert(sizeof(f32) == size);
            *penumbra = set.getPenumbra(id);
        }

        constexpr void getType(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            graphics::light::type* const type{ (graphics::light::type*)data };
            assert(sizeof(graphics::light::type) == size);
            *type = set.getType(id);
        }

        constexpr void getEntityID(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            id::idType* const entityID{ (id::idType*)data };
            assert(sizeof(id::idType) == size);
            *entityID = set.getEntityID(id);
        }

        constexpr void dummySet(lightSet&, lightId, const void* const, u32)
        {

        }

        using setFunction = void(*)(lightSet&, lightId, const void* const, u32);
        using getFunction = void(*)(lightSet&, lightId, void* const, u32);

        constexpr setFunction setFunctions[]
        {
            setIsEnabled,
            setIntensity,
            setColor,
            setAttenuation,
            setRange,
            setUmbra,
            setPenumbra,
            dummySet,
            dummySet
        };

        static_assert(_countof(setFunctions) == lightParameter::count);

        constexpr getFunction getFunctions[]
        {
            getIsEnabled,
            getIntensity,
            getColor,
            getAttenuation,
            getRange,
            getUmbra,
            getPenumbra,
            getType,
            getEntityID,
        };

        static_assert(_countof(getFunctions) == lightParameter::count);

#undef CONSTEXPR
    }

    bool initialize() 
    {
        return true;
    }

    void shutdown()
    {
        assert([] 
        {
            bool hasLights{ false };

            for (const auto& it : lightSets)
            {
                hasLights |= it.second.hasLights();
            }

            return !hasLights;
        }());

        for (u32 i{ 0 }; i < frameBufferCount; ++i)
        {
            lightBuffers[i].release();
        }
    }

    graphics::light create(lightInitInfo info)
    {
		assert(id::isValid(info.entityID));
		return lightSets[info.lightSetKey].add(info);
    }

    void remove(lightId id, u64 lightSetKey)
    {
        assert(lightSets.count(lightSetKey));
        lightSets[lightSetKey].remove(id);
    }

    void setParameter(lightId id, u64 lightSetKey, lightParameter::parameter parameter, const void* const data, u32 dataSize)
    {
        assert(data && dataSize);
        assert(lightSets.count(lightSetKey));
        assert(parameter < lightParameter::count && setFunctions[parameter] != dummySet);

        setFunctions[parameter](lightSets[lightSetKey], id, data, dataSize);
    }

    void getParameter(lightId id, u64 lightSetKey, lightParameter::parameter parameter, void* const data, u32 dataSize)
    {
        assert(data && dataSize);
        assert(lightSets.count(lightSetKey));
        assert(parameter < lightParameter::count);

        getFunctions[parameter](lightSets[lightSetKey], id, data, dataSize);
    }

    void updateLightBuffers(const D3D12FrameInfo& d3D12Info)
    {
        const u64 lightSetKey{ d3D12Info.info->lightSetKey };
        assert(lightSets.count(lightSetKey));

        lightSet& set{ lightSets[lightSetKey] };

        if (!set.hasLights()) return;

        set.updateTransforms();
        const u32 frameIndex{ d3D12Info.frameIndex };
        D3D12LightBuffer& lightBuffer{ lightBuffers[frameIndex] };
        lightBuffer.updateLightBuffers(set, lightSetKey, frameIndex);
    }

    D3D12_GPU_VIRTUAL_ADDRESS getNonCullableLightBuffer(u32 frameIndex)
    {
        const D3D12LightBuffer& lightBuffer{ lightBuffers[frameIndex] };
        return lightBuffer.getNonCullableLights();
    }

    u32 getNonCullableLightCount(u64 lightSetKey)
    {
        assert(lightSets.count(lightSetKey));
        return lightSets[lightSetKey].getNonCullableLightCount();
    }
}