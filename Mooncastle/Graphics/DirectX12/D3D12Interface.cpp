#include "CommonHeaders.h"
#include "D3D12Interface.h"
#include "D3D12Core.h"
#include "D3D12Content.h"
#include "D3D12Camera.h"
#include "D3D12Light.h"
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

		pi.camera.create = camera::create;
		pi.camera.remove = camera::remove;
		pi.camera.setParameter = camera::setParameter;
		pi.camera.getParameter = camera::getParameter;

		pi.light.createLightSet = light::createLightSet;
		pi.light.removeLightSet = light::removeLightSet;
		pi.light.create = light::create;
		pi.light.remove = light::remove;
		pi.light.setParameter = light::setParameter;
		pi.light.getParameter = light::getParameter;

		pi.resources.addSubmesh = content::submesh::add;
		pi.resources.removeSubmesh = content::submesh::remove;
		pi.resources.addMaterial = content::material::add;
		pi.resources.removeMaterial = content::material::remove;
		pi.resources.addRenderItem = content::renderItem::add;
		pi.resources.removeRenderItem = content::renderItem::remove;

        pi.platform = graphicsPlatform::direct3D12;
    }
}