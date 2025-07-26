#include "D3D12Light.h"
#include "Shaders/SharedTypes.h"
#include "D3D12Core.h"
#include "EngineAPI/GameEntity.h"

namespace mooncastle::graphics::d3D12::light 
{
    namespace 
    {
        struct lightOwner
        {
            gameEntity::entityId    entityID{ id::invalidId };
            u32                     dataIndex;
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
                    return {}; //TODO: Add cullable lights.
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
                    //TODO: Add cullable lights.
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

                //TODO: Add cullable lights.
            }

            constexpr void setEnabled(lightId id, bool isEnabled)
            {
                owners[id].isEnabled = isEnabled;

                if (owners[id].type == graphics::light::directional)
                {
                    return;
                }

                //TODO: Add cullable lights.
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
                    //TODO: Add cullable lights.
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
                    //TODO: Add cullable lights.
                }
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
                
				return 0.f; //TODO: Add cullable lights.
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
                
                return {}; //TODO: Add cullable lights.
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

            constexpr bool hasLights() const
            {
                return owners.getSize() > 0;
            }

        private:
            //These containers are NOT tightly packed.
            utl::freeList<lightOwner>                       owners;
            utl::vector<hlsl::DirectionalLightParameters>   nonCullableLights;
            utl::vector<lightId>                            nonCullableOwners;
        };

        class D3D12LightBuffer
        {
        public:
            D3D12LightBuffer() = default;

            CONSTEXPR void updateLightBuffers(lightSet& set, u64 lightSetKey, u32 frameIndex)
            {
                u32 sizes[lightBuffer::count]{};
                sizes[lightBuffer::nonCullableLight] = set.getNonCullableLightCount() * sizeof(hlsl::DirectionalLightParameters);

                u32 currentSizes[lightBuffer::count]{};
                currentSizes[lightBuffer::nonCullableLight] = buffers[lightBuffer::nonCullableLight].buffer.getSize();

                if (currentSizes[lightBuffer::nonCullableLight] < sizes[lightBuffer::nonCullableLight])
                {
                    resizeBuffer(lightBuffer::nonCullableLight, sizes[lightBuffer::nonCullableLight], frameIndex);
                }

                set.setNonCullableLights((hlsl::DirectionalLightParameters* const)buffers[lightBuffer::nonCullableLight].cpuAddress,
                    buffers[lightBuffer::nonCullableLight].buffer.getSize());

                //TODO: Add cullable lights.
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

#undef CONSTEXPR

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

        constexpr void getIsEnabled(lightSet& set, lightId id, void* const data, [[maybe_unused]] u32 size)
        {
            bool* const is_enabled{ (bool* const)data };
            assert(sizeof(bool) == size);
            *is_enabled = set.isEnabled(id);
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
            dummySet,
            dummySet
        };

        static_assert(_countof(setFunctions) == lightParameter::count);

        constexpr getFunction getFunctions[]
        {
            getIsEnabled,
            getIntensity,
            getColor,
            getType,
            getEntityID,
        };

        static_assert(_countof(getFunctions) == lightParameter::count);
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