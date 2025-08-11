#pragma once

#include "CommonHeaders.h"
#include "Platform/Window.h"
#include "EngineAPI/Camera.h"
#include "EngineAPI/Light.h"

namespace mooncastle::graphics 
{
	struct frameInfo
	{
		id::idType*		renderItemIDs{ nullptr };
		f32*			thresholds{ nullptr };
		u64				lightSetKey{ 0 };
		f32				lastFrameTime{ 16.7f };
		f32				averageFrameTime{ 16.7f };
		u32				renderItemCount{ 0 };
		cameraId		cameraID{ id::invalidId };
	};

    DEFINE_TYPED_ID(surfaceId);

    class surface
    {
    public:
        constexpr explicit surface(surfaceId id) : id{ id } {}
        constexpr surface() = default;
        constexpr surfaceId getId() const { return id; }
        constexpr bool isValid() const { return id::isValid(id); }

        void resize(u32 width, u32 height) const;
        u32 width() const;
        u32 height() const;
        void render(frameInfo info) const;
    private:
        surfaceId id{ id::invalidId };
    };

    struct renderSurface
    {
        platform::window               window{};
        mooncastle::graphics::surface  surface{};
    };

	struct directionalLightParameters{};

	struct pointLightParameters
	{
		math::v3 attenuation;
		f32      range;
	};

	struct spotLightParameters
	{
		math::v3 attenuation;
		f32      range;

		//Umbra angle in radians [0, pi)
		f32      umbra;
		//Penumbra angle in radians [umbra, pi)
		f32      penumbra;
	};

	struct ambientParams
	{
		id::idType diffuseTextureID;
		id::idType specularTextureID;
		id::idType brdfLUTTextureID;
	};

	struct lightInitInfo
	{
		u64								lightSetKey{ 0 };
		id::idType						entityID{ id::invalidId };
		light::type						type{};
		f32								intensity{ 1.f };
		math::v3						color{ 1.f, 1.f, 1.f };

		union
		{
			directionalLightParameters	directionalParams;
			pointLightParameters		pointParams;
			spotLightParameters			spotParams;
			ambientParams               ambientParams;
		};

		bool							isEnabled{ true };
	};

	struct lightParameter
	{
		enum parameter : u32
		{
			isEnabled,
			intensity,
			color,
			attenuation,
			range,
			umbra,
			penumbra,
			type,
			entityId,
			count
		};
	};

	struct cameraParameter
	{
		enum parameter : u32
		{
			upVector,
			fieldOfView,
			aspectRatio,
			viewWidth,
			viewHeight,
			nearZ,
			farZ,
			view,
			projection,
			inverseProjection,
			viewProjection,
			inverseViewProjection,
			type,
			entityId,
			count
		};
	};

	struct cameraInitInfo
	{
		id::idType		entityId{ id::invalidId };
		camera::type	type{};
		math::v3		up;
		f32				nearZ;
		f32				farZ;

		union
		{
			f32	        fieldOfView;
			f32	        viewWidth;
		};
		union
		{
			f32	        aspectRatio;
			f32	        viewHeight;
		};
	};

	struct perspectiveCameraInitInfo : public cameraInitInfo
	{
		explicit perspectiveCameraInitInfo(id::idType id)
		{
			assert(id::isValid(id));

			entityId = id;
			type = camera::perspective;
			up = { 0.0f, 1.0f, 0.0f };
			nearZ = 0.1f;
			farZ = 100.0f;
			fieldOfView = 0.25f;
			aspectRatio = 16.0f / 9.0f;
		}
	};

	struct orthographicCameraInitInfo : public cameraInitInfo
	{
		explicit orthographicCameraInitInfo(id::idType id)
		{
			assert(id::isValid(id));

			entityId = id;
			type = camera::orthographic;
			up = { 0.0f, 1.0f, 0.0f };
			nearZ = 0.01f;
			farZ = 1000.0f;
			viewHeight = 1920;
			viewWidth = 1080;
		}
	};

	struct shaderFlags 
	{
		enum flags : u32 
		{
			none = 0x0,
			vertex = 0x01,
			hull = 0x02,
			domain = 0x04,
			geometry = 0x08,
			pixel = 0x10,
			compute = 0x20,
			amplification = 0x40,
			mesh = 0x80,
		};
	};

	struct shaderType
	{
		enum type : u32
		{
			vertex = 0,
			hull,
			domain,
			geometry,
			pixel,
			compute,
			amplification,
			mesh,
			count
		};
	};

	struct materialType
	{
		enum type : u32
		{
			opaque,
			count
		};
	};

	struct materialSurface
	{
		math::v4    baseColor{ 1.f, 1.f, 1.f, 1.f };
		math::v3    emissive{ 0.f, 0.f, 0.f };
		f32         emissiveIntensity{ 1.f };
		f32         metallic{ 0.f };
		f32         roughness{ 1.f };
	};

	struct materialInitInfo
	{
		id::idType*			textureIDs;
		materialSurface     surface;
		materialType::type	type;
		u32					textureCount;
		id::idType			shaderIDs[shaderType::type::count]{ id::invalidId, id::invalidId, id::invalidId, id::invalidId, id::invalidId, id::invalidId, id::invalidId, id::invalidId };
	};

	struct primitiveTopology
	{
		enum type : u32
		{
			pointList = 1,
			lineList,
			lineStrip,
			triangleList,
			triangleStrip,
			count
		};
	};

    enum class graphicsPlatform : u32
    {
        direct3D12 = 0,
    };

    bool initialize(graphicsPlatform platform);
    void shutdown();

    //Get the location of compiled engine shaders relative to the executable's path for the graphics API thatis currently in use.
    const char* getEngineShadersPath();

    //Get the location of compiled engine shaders, for the specified platform, relative to the executable's path for the graphics API that is currently in use.
    const char* getEngineShadersPath(graphicsPlatform platform);

    surface createSurface(platform::window window);
    void removeSurface(surfaceId id);

    camera createCamera(cameraInitInfo info);
    void removeCamera(cameraId id);

	void createLightSet(u64 lightSetKey);
	void removeLightSet(u64 lightSetKey);
    light createLight(lightInitInfo info);
    void removeLight(lightId id, u64 lightSetKey);

    id::idType addSubmesh(const u8*& data);
    void removeSubmesh(id::idType id);

    id::idType addMaterial(materialInitInfo info);
    void removeMaterial(id::idType id);

    id::idType addTexture(const u8 *const data);
    void removeTexture(id::idType id);

	id::idType addRenderItem(id::idType entityID, id::idType geometryContentID, u32 materialCount, const id::idType* const materialIDs);
	void removeRenderItem(id::idType id);
}