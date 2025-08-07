#include "Geometry.h"
#include "Entity.h"
#include "Graphics/Renderer.h"

namespace mooncastle::geometry 
{
    namespace 
    {
        utl::vector<u32>                   activeLOD;
        utl::vector<id::idType>            renderItemIDs;
        utl::vector<geometryId>            ownerIDs;
        utl::vector<id::idType>            idMapping;
        utl::vector<id::generationType>    generations;
        utl::deque<geometryId>             freeIDs;

#if _DEBUG
        bool exists(geometryId id)
        {
            assert(id::isValid(id));

            const id::idType index{ id::index(id) };

            assert(index < generations.size() && !(id::isValid(idMapping[index]) && idMapping[index] >= renderItemIDs.size()));
            assert(generations[index] == id::generation(id));

            return (generations[index] == id::generation(id)) && id::isValid(idMapping[index]) && id::isValid(renderItemIDs[idMapping[index]]);
        }
#endif
    }

    component create(initInfo info, gameEntity::entity entity)
    {
        assert(entity.isValid());
        assert(id::isValid(info.geometryContentID) && info.materialCount && info.materialIDs);

        geometryId id{};

        if (freeIDs.size() > id::minDeletedElements)
        {
            id = freeIDs.front();
            assert(!exists(id));
            freeIDs.pop_front();
            id = geometryId{ id::newGeneration(id) };
            ++generations[id::index(id)];
        }
        else
        {
            id = geometryId{ (id::idType)idMapping.size() };
            idMapping.emplace_back();
            generations.push_back(0);
        }

        assert(id::isValid(id));

        const id::idType index{ (id::idType)renderItemIDs.size() };
        activeLOD.emplace_back(0);
        renderItemIDs.emplace_back(graphics::addRenderItem(entity.getId(), info.geometryContentID, info.materialCount, info.materialIDs));
        ownerIDs.emplace_back(id::index(id));
        idMapping[id::index(id)] = index;

        return component{ id };
    }

    void remove(component c)
    {
        assert(c.isValid() && exists(c.getId()));

        const geometryId id{ c.getId() };
        const id::idType index{ idMapping[id::index(id)] };
        const geometryId lastID{ ownerIDs.back() };

        graphics::removeRenderItem(renderItemIDs[index]);
        utl::erase_unordered(activeLOD, index);
        utl::erase_unordered(renderItemIDs, index);
        utl::erase_unordered(ownerIDs, index);
        idMapping[id::index(lastID)] = index;
        idMapping[id::index(id)] = id::invalidId;

        if (generations[index] < id::maxGeneration)
        {
            freeIDs.push_back(id);
        }
    }

    void getRenderItemIDs(id::idType *const itemIDs, u32 count)
    {
        assert(renderItemIDs.size() >= count);
        memcpy(itemIDs, renderItemIDs.data(), count * sizeof(id::idType));
    }
}