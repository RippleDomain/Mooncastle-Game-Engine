#include "ToolsCommon.h"

namespace mooncastle::tools 
{
    extern void ShutDownTextureTools();
}

EDITOR_INTERFACE void ShutDownContentTools()
{
    using namespace mooncastle::tools;

    ShutDownTextureTools();
}