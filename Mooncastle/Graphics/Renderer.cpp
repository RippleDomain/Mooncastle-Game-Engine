#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "DirectX12\D3D12Interface.h"

namespace mooncastle::graphics
{
    namespace
    {
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
            return true;
        }
    }

    bool initialize(graphicsPlatform platform)
    {
        return setPlatformInterface(platform) && gfx.initialize();
    }

    void shutdown()
    {
        gfx.shutdown();
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