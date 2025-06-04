#include "Entity.h"
#include "Transform.h"

namespace mooncastle::gameEntity 
{
	namespace
	{
		utl::vector<transform::component>     transforms;

		utl::vector<id::generationType>       generations;
		utl::deque<entityId>                  freeIds;
	}

	entity createGameEntity(const entityInfo& info) 
	{
		assert(info.transform); //All components must have a transform.
		if (!info.transform) return entity{};

		entityId id;

		if (freeIds.size() > id::minDeletedElements) 
		{
			id = freeIds.front();
			assert(!isAlive(entity{ id }));
			freeIds.pop_front();
			
			id = entityId{ id::newGeneration(id) };

			++generations[id::index(id)];
		}
		else
		{
			id = entityId{ (id::idType)generations.size() };
			generations.push_back(0);

			//Resize components. (NOT using resize() to avoid more memory allocations)
			transforms.emplace_back();
		}

		const entity newEntity{ id };
		const id::idType index{ id::index(id) };

		//Create the transform component.
		assert(!transforms[index].isValid());
		transforms[index] = transform::createTransform(*info.transform, newEntity);

		if (!transforms[index].isValid()) return {};

		return newEntity;
	}

	void removeGameEntity(entity e)
	{
		const entityId id{ e.getId() };
		const id::idType index{ id::index(id) };

		assert(isAlive(e));

		if (isAlive(e)) 
		{
			transform::removeTransform(transforms[index]);
			transforms[index] = {};
			freeIds.push_back(id);
		}
	}

	bool isAlive(entity e)
	{
		assert(e.isValid());

		const entityId id{ e.getId() };
		const id::idType index{ id::index(id) };

		assert(index < generations.size());
		assert(generations[index] == id::generation(id));

		return (generations[index] == id::generation(id) && transforms[index].isValid());
	}

	transform::component entity::transform() const 
	{
		assert(isAlive(*this));
		const id::idType index{ id::index(_id) };
		
		return transforms[index];
	}
}