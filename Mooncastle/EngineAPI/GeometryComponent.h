#pragma once

#include "../Components/ComponentsCommon.h"

namespace mooncastle::geometry 
{
    DEFINE_TYPED_ID(geometryId);

    class component final
    {
    public:
        constexpr explicit component(geometryId id) : id{ id } {}
        constexpr component() : id{ id::invalidId } {}
        constexpr geometryId getId() const { return id; }
        constexpr bool isValid() const { return id::isValid(id); }

    private:
        geometryId id;
    };
}