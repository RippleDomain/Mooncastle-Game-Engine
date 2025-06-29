#pragma once

namespace mooncastle::graphics 
{
    struct platformInterface;

    namespace d3D12 
    {
        void getPlatformInterface(platformInterface& pi);
    }
}