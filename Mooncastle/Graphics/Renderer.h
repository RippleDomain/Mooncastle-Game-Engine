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

}