#include "Entity.h"
#include "Transform.h"
#include "Script.h"
#include "Geometry.h"

namespace mooncastle::gameEntity 
{
	namespace
	{
		utl::vector<transform::component>     transforms;
		utl::vector<script::component>        scripts;
		utl::vector<geometry::component>      geometries;
		utl::vector<id::generationType>       generations;
		utl::deque<entityId>                  freeIds;
	}

	entity create(entityInfo info) 
	{
		assert(info.transform); //All components must have a transform.
		if (!info.transform) return {};

		entityId id{};

		if (freeIds.size() > id::minDeletedElements) 
		{
			id = freeIds.front();
			assert(!isAlive(id));

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
			scripts.emplace_back();
			geometries.emplace_back();
		}

		const entity newEntity{ id };
		const id::idType index{ id::index(id) };

		//Create the transform component.
		assert(!transforms[index].isValid());
		transforms[index] = transform::create(*info.transform, newEntity);
		assert(transforms[index].getId() == id);

		if (!transforms[index].isValid()) return {};

		//Create the script component.
		if (info.script && info.script->scriptCreator) 
		{
			assert(!scripts[index].isValid());
			scripts[index] = script::create(*info.script, newEntity);
			assert(scripts[index].isValid());
		}

		//Create the geometry component.
		if (info.geometry) 
		{
			assert(!geometries[index].isValid());
			geometries[index] = geometry::create(*info.geometry, newEntity);
			assert(geometries[index].isValid());
		}

		return newEntity;
	}

	void remove(entityId id)
	{
		const id::idType index{ id::index(id) };
		assert(isAlive(id));

		if (geometries[index].isValid())
		{
			geometry::remove(geometries[index]);
			geometries[index] = {};
		}

		if (scripts[index].isValid())
		{
			script::remove(scripts[index]);
			scripts[index] = {};
		}

		transform::remove(transforms[index]);
		transforms[index] = {};

		if (generations[index] < id::maxGeneration)
		{
			freeIds.push_back(id);
		}
	}

	bool isAlive(entityId id)
	{
		assert(id::isValid(id));
		const id::idType index{ id::index(id) };
		assert(index < generations.size());
		assert(generations[index] == id::generation(id));
		return generations[index] == id::generation(id) && transforms[index].isValid();
	}

	transform::component entity::transform() const
	{
		assert(isAlive(id));
		return transforms[id::index(id)];
	}

	script::component entity::script() const
	{
		assert(isAlive(id));
		return scripts[id::index(id)];
	}

	geometry::component entity::geometry() const
	{
		assert(isAlive(id));
		return geometries[id::index(id)];
	}
}