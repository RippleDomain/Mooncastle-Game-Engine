#include "CommonHeaders.h"
#include "D3D12Interface.h"
#include "Graphics/GraphicsPlatformInterface.h"
#include "D3D12Core.h"

namespace mooncastle::graphics::d3D12 
{
    void getPlatformInterface(platformInterface& pi)
    {
        pi.initialize = core::initialize;
        pi.shutdown = core::shutdown;
        pi.render = core::render;
    }
}