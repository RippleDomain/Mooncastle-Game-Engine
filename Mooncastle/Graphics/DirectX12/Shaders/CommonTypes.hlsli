#if !defined(MOONCASTLE_COMMON_HLSLI) && !defined(__cplusplus)

#error Do not include this file directly in the shader files. Only do it via Common.hlsli.

#endif

struct GlobalShaderData
{
    float4x4    View;
    float4x4    Projection;
    float4x4    InvProjection;
    float4x4    ViewProjection;
    float4x4    InvViewProjection;

    float3      CameraPosition;
    float       ViewWidth;

    float3      CameraDirection;
    float       ViewHeight;

    uint        NumDirectionalLights;
    float       DeltaTime;
};

struct PerObjectData
{
    float4x4    World;
    float4x4    InvWorld;
    float4x4    WorldViewProjection;
};

struct LightCullingLightInfo
{
    float3 Position;
    float Range;

    float3 Direction;
    float ConeRadius;

    uint Type;
    float3 Pad;
};

struct LightParameters
{
    float3 Position;
    float Intensity;

    float3 Direction;
    uint Type;

    float3 Color;
    float Range;

    float3 Attenuation;
    float CosUmbra;

    float CosPenumbra;
    float3 Pad;
};

struct DirectionalLightParameters
{
    float3      Direction;
    float       Intensity;
    float3      Color;
    float       Pad;
};

#ifdef __cplusplus

static_assert((sizeof(PerObjectData) % 16) == 0, "Make sure PerObjectData is formatted in 16-byte chunks without any implicit padding.");
static_assert((sizeof(LightParameters) % 16) == 0, "Make sure LightParameters is formatted in 16-byte chunks without any implicit padding.");
static_assert((sizeof(LightCullingLightInfo) % 16) == 0, "Make sure LightCullingLightInfo is formatted in 16-byte chunks without any implicit padding.");
static_assert((sizeof(DirectionalLightParameters) % 16) == 0, "Make sure DirectionalLightParameters is formatted in 16-byte chunks without any implicit padding.");

#endif
