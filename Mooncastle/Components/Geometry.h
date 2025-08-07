#pragma once

#include "ComponentsCommon.h"

namespace mooncastle::geometry 
{
    struct initInfo
    {
        id::idType     geometryContentID;
        u32            materialCount;
        id::idType*    materialIDs;
    };

    component create(initInfo info, gameEntity::entity entity);
    void remove(component c);
    void getRenderItemIDs(id::idType *const itemIDs, u32 count);
}