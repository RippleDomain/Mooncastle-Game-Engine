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

    void render()
    {
        gfx.render();
    }
}