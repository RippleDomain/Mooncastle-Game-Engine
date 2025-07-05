#pragma once
#include "CommonHeaders.h"
#include "..\Platform\Window.h"

namespace mooncastle::graphics 
{
    DEFINE_TYPED_ID(surfaceId);

    class surface
    {
    public:
        constexpr explicit surface(surfaceId id) : id{ id } {}
        constexpr surface() = default;
        constexpr surfaceId getId() const { return id; }
        constexpr bool isValid() const { return id::isValid(id); }

        void resize(u32 width, u32 height) const;
        u32 width() const;
        u32 height() const;
        void render() const;
    private:
        surfaceId id{ id::invalidId };
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

    //Get the location of compiled engine shaders relative to the executable's path for the graphics API thatis currently in use.
    const char* getEngineShadersPath();

    //Get the location of compiled engine shaders, for the specified platform, relative to the executable's path for the graphics API that is currently in use.
    const char* getEngineShadersPath(graphicsPlatform platform);

    surface createSurface(platform::window window);
    void removeSurface(surfaceId id);
}