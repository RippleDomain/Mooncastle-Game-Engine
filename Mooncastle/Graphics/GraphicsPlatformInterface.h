#pragma once

#include "CommonHeaders.h"
#include "Renderer.h"

namespace mooncastle::graphics 
{
    struct platformInterface
    {
        bool(*initialize)(void);
        void(*shutdown)(void);

        struct 
        {
            surface(*create)(platform::window);
            void (*remove)(surfaceId id);
            void (*resize)(surfaceId id, u32 width, u32 height);
            u32(*getWidth)(surfaceId id);
            u32(*getHeight)(surfaceId id);
            void (*render)(surfaceId id, frameInfo);
        } surface;

        struct
        {
            id::idType(*addSubmesh)(const u8*&);
            void (*removeSubmesh)(id::idType);

            id::idType(*addMaterial)(materialInitInfo);
            void (*removeMaterial)(id::idType);

            id::idType(*addRenderItem)(id::idType, id::idType, u32, const id::idType* const);
            void(*removeRenderItem)(id::idType);
        } resources;

        struct
        {
            camera(*create)(cameraInitInfo);
            void(*remove)(cameraId);
            void(*setParameter)(cameraId, cameraParameter::parameter, const void *const, u32);
            void(*getParameter)(cameraId, cameraParameter::parameter, void *const, u32);
        } camera;

        struct
        {
            void(*createLightSet)(u64);
            void(*removeLightSet)(u64);
            light(*create)(lightInitInfo);
            void(*remove)(lightId, u64);
            void(*setParameter)(lightId, u64, lightParameter::parameter, const void *const, u32);
            void(*getParameter)(lightId, u64, lightParameter::parameter, void *const, u32);
        } light;

        graphicsPlatform platform = (graphicsPlatform)-1;
    };
}