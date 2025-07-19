#pragma once
#include "CommonHeaders.h"
#include "Platform/Window.h"
#include "EngineAPI/Camera.h"

namespace mooncastle::graphics 
{
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
        void render() const;
    private:
        surfaceId id{ id::invalidId };
    };

    struct renderSurface
    {
        platform::window               window{};
        mooncastle::graphics::surface  surface{};
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
			nearZ = 0.01f;
			farZ = 1000.0f;
			fieldOfView = 0.25f;
			aspectRatio = 16.0f / 10.0f;
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

    id::idType addSubmesh(const u8*& data);
    void removeSubmesh(id::idType id);
}