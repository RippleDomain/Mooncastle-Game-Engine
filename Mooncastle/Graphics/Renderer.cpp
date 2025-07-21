#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "DirectX12\D3D12Interface.h"

namespace mooncastle::graphics
{
    namespace
    {
        //Defines where the compiled engine shaders file is located for each one of the supported APIs.
        constexpr const char* engineShaderPaths[]
        {
            ".\\Shaders\\D3D12\\shaders.bin"
        };

        platformInterface gfx{};

        bool setPlatformInterface(graphicsPlatform platform)
        {
            switch (platform)
            {
            case graphicsPlatform::direct3D12:
                d3D12::getPlatformInterface(gfx); 
                break;
            default:
                return false;
            }

            assert(gfx.platform == platform);

            return true;
        }
    }

    bool initialize(graphicsPlatform platform)
    {
        return setPlatformInterface(platform) && gfx.initialize();
    }

    void shutdown()
    {
        if (gfx.platform != (graphicsPlatform)-1) gfx.shutdown();
    }

    const char* getEngineShadersPath()
    {
        return engineShaderPaths[(u32)gfx.platform];
    }

    const char* getEngineShadersPath(graphicsPlatform platform)
    {
        return engineShaderPaths[(u32)platform];
    }

    surface createSurface(platform::window window)
    {
        return gfx.surface.create(window);
    }

    void removeSurface(surfaceId id)
    {
        assert(id::isValid(id));
        gfx.surface.remove(id);
    }

    camera createCamera(cameraInitInfo info)
    {
        return gfx.camera.create(info);
    }

    void removeCamera(cameraId id)
    {
        gfx.camera.remove(id);
    }

	void camera::up(math::v3 up) const
	{
		assert(isValid());

		gfx.camera.setParameter(id, cameraParameter::upVector, &up, sizeof(up));
	}

	void camera::fieldOfView(f32 fov) const
	{
		assert(isValid());

		gfx.camera.setParameter(id, cameraParameter::fieldOfView, &fov, sizeof(fov));
	}

	void camera::aspectRatio(f32 aspect_ratio) const
	{
		assert(isValid());

		gfx.camera.setParameter(id, cameraParameter::aspectRatio, &aspect_ratio, sizeof(aspect_ratio));
	}

	void camera::viewWidth(f32 width) const
	{
		assert(isValid());

		gfx.camera.setParameter(id, cameraParameter::viewWidth, &width, sizeof(width));
	}

	void camera::viewHeight(f32 height) const
	{
		assert(isValid());

		gfx.camera.setParameter(id, cameraParameter::viewHeight, &height, sizeof(height));
	}

	void camera::range(f32 near_z, f32 far_z) const
	{
		assert(isValid());

		gfx.camera.setParameter(id, cameraParameter::nearZ, &near_z, sizeof(near_z));
		gfx.camera.setParameter(id, cameraParameter::farZ, &far_z, sizeof(far_z));
	}

	math::m4x4 camera::view() const
	{
		assert(isValid());

		math::m4x4 matrix;
		gfx.camera.getParameter(id, cameraParameter::view, &matrix, sizeof(matrix));

		return matrix;
	}

	math::m4x4 camera::projection() const
	{
		assert(isValid());

		math::m4x4 matrix;
		gfx.camera.getParameter(id, cameraParameter::projection, &matrix, sizeof(matrix));

		return matrix;
	}

	math::m4x4 camera::inverseProjection() const
	{
		assert(isValid());

		math::m4x4 matrix;
		gfx.camera.getParameter(id, cameraParameter::inverseProjection, &matrix, sizeof(matrix));

		return matrix;
	}

	math::m4x4 camera::viewProjection() const
	{
		assert(isValid());

		math::m4x4 matrix;
		gfx.camera.getParameter(id, cameraParameter::viewProjection, &matrix, sizeof(matrix));

		return matrix;
	}

	math::m4x4 camera::inverseViewProjection() const
	{
		assert(isValid());

		math::m4x4 matrix;
		gfx.camera.getParameter(id, cameraParameter::inverseViewProjection, &matrix, sizeof(matrix));

		return matrix;
	}

	math::v3 camera::up() const
	{
		assert(isValid());

		math::v3 upVector;
		gfx.camera.getParameter(id, cameraParameter::upVector, &upVector, sizeof(upVector));

		return upVector;
	}

	f32 camera::nearZ() const
	{
		assert(isValid());

		f32 nearZ;
		gfx.camera.getParameter(id, cameraParameter::nearZ, &nearZ, sizeof(nearZ));

		return nearZ;
	}

	f32 camera::farZ() const
	{
		assert(isValid());

		f32 farZ;
		gfx.camera.getParameter(id, cameraParameter::farZ, &farZ, sizeof(farZ));

		return farZ;
	}

	f32 camera::fieldOfView() const
	{
		assert(isValid());

		f32 fov;
		gfx.camera.getParameter(id, cameraParameter::fieldOfView, &fov, sizeof(fov));

		return fov;
	}

	f32 camera::aspectRatio() const
	{
		assert(isValid());

		f32 aspectRatio;
		gfx.camera.getParameter(id, cameraParameter::aspectRatio, &aspectRatio, sizeof(aspectRatio));

		return aspectRatio;
	}

	f32 camera::viewWidth() const
	{
		assert(isValid());

		f32 width;
		gfx.camera.getParameter(id, cameraParameter::viewWidth, &width, sizeof(width));

		return width;
	}

	f32 camera::viewHeight() const
	{
		assert(isValid());

		f32 height;
		gfx.camera.getParameter(id, cameraParameter::viewHeight, &height, sizeof(height));

		return height;
	}

	camera::type camera::projectionType() const
	{
		assert(isValid());

		type type;
		gfx.camera.getParameter(id, cameraParameter::type, &type, sizeof(type));

		return type;
	}

	id::idType camera::entityId() const
	{
		assert(isValid());

		id::idType idToGet;
		gfx.camera.getParameter(id, cameraParameter::entityId, &idToGet, sizeof(idToGet));

		return idToGet;
	}

    void surface::resize(u32 width, u32 height) const
    {
        assert(isValid());
        gfx.surface.resize(id, width, height);
    }

    u32 surface::width() const
    {
        assert(isValid());
        return gfx.surface.getWidth(id);
    }

    u32 surface::height() const
    {
        assert(isValid());
        return gfx.surface.getHeight(id);
    }

    void surface::render() const
    {
        assert(isValid());
        gfx.surface.render(id);
    }

    id::idType addSubmesh(const u8*& data)
    {
        return gfx.resources.addSubmesh(data);
    }

    void removeSubmesh(id::idType id)
    {
        gfx.resources.removeSubmesh(id);
    }

	id::idType addMaterial(materialInitInfo info)
	{
		return gfx.resources.addMaterial(info);
	}

	void removeMaterial(id::idType id)
	{
		gfx.resources.removeMaterial(id);
	}

	id::idType addRenderItem(id::idType entityID, id::idType geometryContentID, u32 materialCount, const id::idType* const materialIDs)
	{
		return gfx.resources.addRenderItem(entityID, geometryContentID, materialCount, materialIDs);
	}

	void removeRenderItem(id::idType id)
	{
		gfx.resources.removeRenderItem(id);
	}
}