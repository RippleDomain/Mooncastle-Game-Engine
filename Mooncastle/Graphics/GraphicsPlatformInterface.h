#pragma once

#include "CommonHeaders.h"
#include "Renderer.h"
#include "Platform/Window.h"

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
            void (*render)(surfaceId id);
        } surface;

        struct
        {
            id::idType(*addSubmesh)(const u8*&);
            void (*removeSubmesh)(id::idType);
        } resources;

        graphicsPlatform platform = (graphicsPlatform)-1;
    };
}