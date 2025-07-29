#include "Common.hlsli"

#if USE_BOUNDING_SPHERES

static const uint MaxLightsPerGroup = 1024;

groupshared uint  minDepthVS;                               //Tile's minimum depth in the view-space.
groupshared uint  maxDepthVS;                               //Tile's maximum depth in the view-space.
groupshared uint  lightCount;                               //Number of lights that affect the pixels in this tile.
groupshared uint  lightIndexStartOffset;                    //Offset in the global light index list where we copy lightIndexList.
groupshared uint  lightIndexList[MaxLightsPerGroup];        //Indices of lights that affect this tile.
groupshared uint  lightFlagsOpaque[MaxLightsPerGroup];      //Flags the lights in the tile that are actually affecting pixels.
groupshared uint  spotlightStartOffset;
groupshared uint2 opaqueLightIndex;                         //X for point lights and Y for spotlights.

ConstantBuffer<GlobalShaderData>                GlobalData              :   register(b0, space0);
ConstantBuffer<LightCullingDispatchParameters>  ShaderParams            :   register(b1, space0);
StructuredBuffer<Frustum>                       Frustums                :   register(t0, space0);
StructuredBuffer<LightCullingLightInfo>         Lights                  :   register(t1, space0);
StructuredBuffer<Sphere>                        BoundingSpheres         :   register(t2, space0);

RWStructuredBuffer<uint>                        LightIndexCounter       :   register(u0, space0);
RWStructuredBuffer<uint2>                       LightGrid_Opaque        :   register(u1, space0);
RWStructuredBuffer<uint>                        LightIndexList_Opaque   :   register(u3, space0);

Sphere GetConeBoundingSphere(float3 tip, float range, float3 direction, float cosPenumbra)
{
    Sphere sphere;
    sphere.Radius = range / (2.0f * cosPenumbra);
    sphere.Center = tip + sphere.Radius * direction;

    if (cosPenumbra < 0.707107f /*cos45*/)
    {
        const float coneSin = sqrt(1.f - cosPenumbra * cosPenumbra);
        sphere.Center = tip + cosPenumbra * range * direction;
        sphere.Radius = coneSin * range;
    }
    
    return sphere;
}

bool Intersects(Frustum frustum, Sphere sphere, float minDepth, float maxDepth)
{
    if ((sphere.Center.z - sphere.Radius > minDepth) || (sphere.Center.z + sphere.Radius < maxDepth))
    {
        return false;
    }

    const float3 lightRejection = sphere.Center - dot(sphere.Center, frustum.ConeDirection) * frustum.ConeDirection;
    const float distSq = dot(lightRejection, lightRejection);
    const float radius = sphere.Center.z * frustum.UnitRadius + sphere.Radius;
    const float radiusSq = radius * radius;

    return distSq <= radiusSq;
}

//TILE_SIZE is defined by the engine at compile time.
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CullLightsCS(ComputeShaderInput csInput)
{    
    //----------- STEP 1 : INITIALIZATION -----------//
    const float depth = Texture2D( ResourceDescriptorHeap[ShaderParams.DepthBufferSrvIndex])[csInput.DispatchThreadID.xy].r;
    const float C = GlobalData.Projection._m22;
    const float D = GlobalData.Projection._m23;
    const uint gridIndex = csInput.GroupID.x + (csInput.GroupID.y * ShaderParams.NumThreadGroups.x);
    const Frustum frustum = Frustums[gridIndex];
    
    if (csInput.GroupIndex == 0) //Only the first thread in the group need to initialize groupshared memory.
    {
        minDepthVS = 0x7f7fffff; //FLT_MAX as uint.
        maxDepthVS = 0;
        lightCount = 0;
        opaqueLightIndex = 0;
    }

    uint i = 0;
    uint index = 0;
    
    for (i = csInput.GroupIndex; i < MaxLightsPerGroup; i += TILE_SIZE * TILE_SIZE)
    {
        lightFlagsOpaque[i] = 0;
    }

    //----------- STEP 2 : DEPTH MIN/MAX -----------//
    GroupMemoryBarrierWithGroupSync();

    if (depth != 0) //Do not include the far plane.
    {
        //Swap min/max because of reversed depth.
        const float depthMin = WaveActiveMax(depth);
        const float depthMax = WaveActiveMin(depth);

        if (WaveIsFirstLane())
        {
            //Negate depth because of right-handed coordinates (negative z-axis).
            //This makes the comparisons easier to understand.
            const uint zMin = asuint(D / (depthMin + C)); //-minDepthVS as uint.
            const uint zMax = asuint(D / (depthMax + C)); //-maxDepthVS as uint.
            
            InterlockedMin(minDepthVS, zMin);
            InterlockedMax(maxDepthVS, zMax);
        }
    }

    //----------- STEP 3 : LIGHT CULLING -----------//
    GroupMemoryBarrierWithGroupSync();
    
    //Negate view-space min/max again to end up with negative z values.
    const float newMinDepthVS = -asfloat(minDepthVS);
    const float newMaxDepthVS = -asfloat(maxDepthVS);

    for (i = csInput.GroupIndex; i < ShaderParams.NumLights; i += TILE_SIZE * TILE_SIZE)
    {
        Sphere sphere = BoundingSpheres[i];
        sphere.Center = mul(GlobalData.View, float4(sphere.Center, 1.f)).xyz;

        if (Intersects(frustum, sphere, newMinDepthVS, newMaxDepthVS))
        {
            InterlockedAdd(lightCount, 1, index);
            if (index < MaxLightsPerGroup)
            {
                lightIndexList[index] = i;
            }
        }
    }

    //----------- STEP 4 : LIGHT PRUNING -----------//
    GroupMemoryBarrierWithGroupSync();
    
    const uint newLightCount = min(lightCount, MaxLightsPerGroup);
    const float2 invViewDimensions = 1.f / float2(GlobalData.ViewWidth, GlobalData.ViewHeight);

    //Gets the world position of this pixel.
    const float3 pos = UnprojectUV(csInput.DispatchThreadID.xy * invViewDimensions, depth, GlobalData.InvViewProjection).xyz;

    for (i = 0; i < newLightCount; ++i)
    {
        index = lightIndexList[i];
        const LightCullingLightInfo light = Lights[index];
        const float3 d = pos - light.Position;
        const float distSq = dot(d, d);

        if (distSq <= light.Range * light.Range)
        {
        //-1 means the light is a point light. It's a spotlight otherwise.
            const bool isPointLight = light.CosPenumbra == -1.f;
            if (isPointLight || (dot(d * rsqrt(distSq), light.Direction) >= light.CosPenumbra))
            {
                lightFlagsOpaque[i] = 2 - uint(isPointLight);
            }
        }
    }

    //----------- STEP 5 : UPDATE LIGHT GRID -----------//
    GroupMemoryBarrierWithGroupSync();
    
    if (csInput.GroupIndex == 0)
    {
        uint numPointLights = 0;
        uint numSpotlights = 0;

        for (i = 0; i < newLightCount; ++i)
        {
            numPointLights += (lightFlagsOpaque[i] & 1);
            numSpotlights += (lightFlagsOpaque[i] >> 1);
        }

        InterlockedAdd(LightIndexCounter[0], numPointLights + numSpotlights, lightIndexStartOffset);
        spotlightStartOffset = lightIndexStartOffset + numPointLights;
        LightGrid_Opaque[gridIndex] = uint2(lightIndexStartOffset, (numPointLights << 16) | numSpotlights);
    }
    
    //----------- STEP 6 : UPDATE LIGHT INDEX LIST -----------//
    GroupMemoryBarrierWithGroupSync();
    
    uint pointIndex, spotIndex;

    for (i = csInput.GroupIndex; i < newLightCount; i += TILE_SIZE * TILE_SIZE)
    {
        if (lightFlagsOpaque[i] == 1)
        {
            InterlockedAdd(opaqueLightIndex.x, 1, pointIndex);
            LightIndexList_Opaque[lightIndexStartOffset + pointIndex] = lightIndexList[i];
        }
        else if (lightFlagsOpaque[i] == 2)
        {
            InterlockedAdd(opaqueLightIndex.y, 1, spotIndex);
            LightIndexList_Opaque[spotlightStartOffset + spotIndex] = lightIndexList[i];
        }
    }
}

#else

static const uint MaxLightsPerGroup = 1024;

groupshared uint minDepthVS;                        //Tile's minimum depth in the view-space.
groupshared uint maxDepthVS;                        //Tile's maximum depth in the view-space.
groupshared uint lightCount;                        //Number of lights that affect the pixels in this tile.
groupshared uint lightIndexStartOffset;             //Offset in the global light index list where we copy lightIndexList.
groupshared uint lightIndexList[MaxLightsPerGroup]; //Indices of lights that affect this tile.

ConstantBuffer<GlobalShaderData>                GlobalData              :   register(b0, space0);
ConstantBuffer<LightCullingDispatchParameters>  ShaderParams            :   register(b1, space0);
StructuredBuffer<Frustum>                       Frustums                :   register(t0, space0);
StructuredBuffer<LightCullingLightInfo>         Lights                  :   register(t1, space0);

RWStructuredBuffer<uint>                        LightIndexCounter       :   register(u0, space0);
RWStructuredBuffer<uint2>                       LightGrid_Opaque        :   register(u1, space0);
RWStructuredBuffer<uint>                        LightIndexList_Opaque   :   register(u3, space0);

//"Forward vs Deffered vs Forward+ Rendering with DirectX 11" (2005) by Jeremiah van Oosten.
//https://www.3dgep.com/forward-plus/#light-culling

//TILE_SIZE is defined by the engine at compile time.
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CullLightsCS(ComputeShaderInput csInput)
{
    //----------- STEP 1 : INITIALIZATION -----------//
    if (csInput.GroupIndex == 0) //Only the first thread in the group need to initialize groupshared memory.
    {
        minDepthVS = 0x7f7fffff; //FLT_MAX as uint.
        maxDepthVS = 0;
        lightCount = 0;
    }

    uint i = 0;
    uint index = 0;

    //----------- STEP 2 : DEPTH MIN/MAX -----------//
    GroupMemoryBarrierWithGroupSync();

    const float depth = Texture2D(ResourceDescriptorHeap[ShaderParams.DepthBufferSrvIndex])[csInput.DispatchThreadID.xy].r;
    const float depthVS = ClipToView(float4(0.f, 0.f, depth, 1.f), GlobalData.InvProjection).z;
    
    //Negate depth value because of right-handed coordinates. This makes the comparisons easier to understand.
    const uint z = asuint(-depthVS);

    if (depth != 0) //Don't include the far plane.
    {
        InterlockedMin(minDepthVS, z);
        InterlockedMax(maxDepthVS, z);
    }

    //----------- STEP 3 : LIGHT CULLING -----------//
    GroupMemoryBarrierWithGroupSync();
    
    const uint gridIndex = csInput.GroupID.x + (csInput.GroupID.y * ShaderParams.NumThreadGroups.x);
    const Frustum frustum = Frustums[gridIndex];
    
    //Negate view-space min/max again to end up with negative z values.
    const float newMinDepthVS = -asfloat(minDepthVS);
    const float newMaxDepthVS = -asfloat(maxDepthVS);

    for (i = csInput.GroupIndex; i < ShaderParams.NumLights; i += TILE_SIZE * TILE_SIZE)
    {
        const LightCullingLightInfo light = Lights[i];
        const float3 lightPositionVS = mul(GlobalData.View, float4(light.Position, 1.f)).xyz;

        if (light.Type == LIGHT_TYPE_POINT_LIGHT)
        {
            const Sphere sphere = { lightPositionVS, light.Range };
            
            if (SphereInsideFrustum(sphere, frustum, newMinDepthVS, newMaxDepthVS))
            {
                InterlockedAdd(lightCount, 1, index);
                
                if (index < MaxLightsPerGroup)
                {
                    lightIndexList[index] = i;
                }
            }
        }
        else if (light.Type == LIGHT_TYPE_SPOTLIGHT)
        {
            const float3 lightDirectionVS = mul(GlobalData.View, float4(light.Direction, 0.f)).xyz;
            const Cone cone = { lightPositionVS, light.Range, lightDirectionVS, light.ConeRadius };
            
            if (ConeInsideFrustum(cone, frustum, newMinDepthVS, newMaxDepthVS))
            {
                InterlockedAdd(lightCount, 1, index);
                
                if (index < MaxLightsPerGroup)
                {
                    lightIndexList[index] = i;
                }
            }
        }
    }

    //----------- STEP 4 : UPDATE LIGHT GRID -----------//
    GroupMemoryBarrierWithGroupSync();
    
    const uint newLightCount = min(lightCount, MaxLightsPerGroup);

    if (csInput.GroupIndex == 0)
    {
        InterlockedAdd(LightIndexCounter[0], newLightCount, lightIndexStartOffset);
        LightGrid_Opaque[gridIndex] = uint2(lightIndexStartOffset, newLightCount);
    }

    //----------- STEP 5 : UPDATE LIGHT INDEX LIST -----------//
    GroupMemoryBarrierWithGroupSync();
    
    for (i = csInput.GroupIndex; i < newLightCount; i += TILE_SIZE * TILE_SIZE)
    {
        LightIndexList_Opaque[lightIndexStartOffset + i] = lightIndexList[i];
    }
}

#endif