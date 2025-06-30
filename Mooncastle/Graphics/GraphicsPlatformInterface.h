#pragma once

#include "CommonHeaders.h"
#include "Renderer.h"

namespace mooncastle::graphics 
{
    struct platformInterface
    {
        bool(*initialize)(void);
        void(*shutdown)(void);
        void(*render)(void);
    };
}