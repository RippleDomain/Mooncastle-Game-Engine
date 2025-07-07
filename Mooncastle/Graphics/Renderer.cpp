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
}