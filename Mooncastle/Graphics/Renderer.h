#pragma once
#include "CommonHeaders.h"
#include "..\Platform\Window.h"

namespace mooncastle::graphics {

    class surface
    {

    };

    struct renderSurface
    {
        platform::window window{};
        surface surface{};
    };

    enum class graphicsPlatform : u32
    {
        direct3D12 = 0,
    };

    bool initialize(graphicsPlatform platform);
    void shutdown();
}