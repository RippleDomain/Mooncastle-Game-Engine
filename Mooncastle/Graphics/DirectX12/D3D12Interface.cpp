#include "CommonHeaders.h"
#include "D3D12Interface.h"
#include "D3D12Core.h"
#include "D3D12Content.h"
#include "Graphics/GraphicsPlatformInterface.h"

namespace mooncastle::graphics::d3D12 
{
    void getPlatformInterface(platformInterface& pi)
    {
        pi.initialize = core::initialize;
        pi.shutdown = core::shutdown;

        pi.surface.create = core::createSurface;
        pi.surface.remove = core::removeSurface;
        pi.surface.resize = core::resizeSurface;
        pi.surface.getWidth = core::surfaceWidth;
        pi.surface.getHeight = core::surfaceHeight;
        pi.surface.render = core::renderSurface;

		pi.resources.addSubmesh = content::submesh::add;
		pi.resources.removeSubmesh = content::submesh::remove;

        pi.platform = graphicsPlatform::direct3D12;
    }
}