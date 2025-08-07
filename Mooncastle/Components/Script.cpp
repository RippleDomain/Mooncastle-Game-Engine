#include "Script.h"
#include "Entity.h"
#include "Transform.h"

#define USE_TRANSFORM_CACHE_MAP 0

namespace mooncastle::script 
{
	namespace
	{
		utl::vector<detail::script_ptr>			entityScripts;
		utl::vector<id::idType>					idMapping;
		utl::vector<id::generationType>			generations;
		utl::deque <scriptId>					freeIds;
		utl::vector<transform::componentCache>	transformCache;

#if USE_TRANSFORM_CACHE_MAP
		std::unordered_map<id::idType, u32>     cacheMap;
#endif

		using scriptRegistery = std::unordered_map<size_t, detail::script_creator>;

		scriptRegistery& registery() 
		{
			static scriptRegistery reg;
			return reg;
		}

#ifdef USE_WITH_EDITOR
		utl::vector<std::string>& scriptNames()
		{
			static utl::vector<std::string> names;
			return names;
		}

#endif

#if _DEBUG
		
		bool exists(scriptId id) 
		{
			assert(id::isValid(id));

			const id::idType index{ id::index(id) };
			assert(index < generations.size() && !(id::isValid(idMapping[index]) && idMapping[index] >= entityScripts.size()));
			assert(generations[index] == id::generation(id));

			return (id::isValid(idMapping[index]) &&
				generations[index] == id::generation(id)) &&
				entityScripts[idMapping[index]] &&
				entityScripts[idMapping[index]]->isValid();
		};

#endif

#if USE_TRANSFORM_CACHE_MAP
		transform::componentCache* const getCachePointer(const gameEntity::entity* const entity)
		{
			assert(gameEntity::isAlive((*entity).getId()));

			const transform::transformId id{ (*entity).transform().getId() };

			u32 index{ u32_invalid_id };
			auto pair = cacheMap.try_emplace(id, id::invalidId);

			//cacheMap didn't have an entry for this ID, so we create a new entry.
			if (pair.second)
			{
				index = (u32)transformCache.size();
				transformCache.emplace_back();
				transformCache.back().id = id;
				cacheMap[id] = index;
			}
			else
			{
				index = cacheMap[id];
			}

			assert(index < transformCache.size());

			return &transformCache[index];
		}
#else
		transform::componentCache* const getCachePointer(const gameEntity::entity* const entity)
		{
			assert(gameEntity::isAlive((*entity).getId()));

			const transform::transformId id{ (*entity).transform().getId() };

			for (auto& cache : transformCache)
			{
				if (cache.id == id)
				{
					return &cache;
				}
			}

			transformCache.emplace_back();
			transformCache.back().id = id;

			return &transformCache.back();
		}
#endif

	}

	namespace detail
	{
		u8 registerScript(size_t tag, script_creator func) 
		{
			bool result{ registery().insert(scriptRegistery::value_type{tag, func}).second };
			assert(result);
			return result;
		}

		script_creator getScriptCreator(size_t tag) 
		{
			auto script = mooncastle::script::registery().find(tag);
			assert(script != mooncastle::script::registery().end() && script->first == tag);
			return script->second;
		}

		#ifdef USE_WITH_EDITOR
		u8 addScriptName(const char* name)
		{
			scriptNames().emplace_back(name);
			return true;
		}

		#endif

	}

	component create(initInfo info, gameEntity::entity entity)
	{
		assert(entity.isValid());
		assert(info.scriptCreator);
		scriptId id{};

		if (freeIds.size() > id::minDeletedElements)
		{
			id = freeIds.front();
			assert(!exists(id));

			freeIds.pop_front();
			id = scriptId{ id::newGeneration(id) };
			++generations[id::index(id)];
		}
		else
		{
			id = scriptId{ (id::idType)idMapping.size() };
			idMapping.emplace_back();
			generations.push_back(0);
		}
		assert(id::isValid(id));

		const id::idType index{ (id::idType)entityScripts.size() };
		entityScripts.emplace_back(info.scriptCreator(entity));
		assert(entityScripts.back()->getId() == entity.getId());
		idMapping[id::index(id)] = index;

		return component{ id };
	}

	void remove(component c)
	{
		assert(c.isValid() && exists(c.getId()));

		const scriptId id{ c.getId() };
		const id::idType index{ idMapping[id::index(id)] };
		const scriptId lastId{ entityScripts.back()->script().getId()};

		utl::erase_unordered(entityScripts, index);
		idMapping[id::index(lastId)] = index;
		idMapping[id::index(id)] = id::invalidId;

		if (generations[index] < id::maxGeneration)
		{
			freeIds.push_back(id);
		}
	}

	void update(f32 dt)
	{
		for (const auto& ptr : entityScripts)
		{
			ptr->update(dt);
		}

		if (transformCache.size())
		{
			transform::update(transformCache.data(), (u32)transformCache.size());
			transformCache.clear();

#if USE_TRANSFORM_CACHE_MAP
			cacheMap.clear();
#endif
		}
	}

	void entityScript::setRotation(const gameEntity::entity *const entity, math::v4 rotationQuaternion)
	{
		transform::componentCache& cache{ *getCachePointer(entity) };
		cache.flags |= transform::componentFlags::rotation;
		cache.rotation = rotationQuaternion;
	}

	void entityScript::setOrientation(const gameEntity::entity *const entity, math::v3 orientationVector)
	{
		transform::componentCache& cache{ *getCachePointer(entity) };
		cache.flags |= transform::componentFlags::orientation;
		cache.orientation = orientationVector;
	}

	void entityScript::setPosition(const gameEntity::entity *const entity, math::v3 position)
	{
		transform::componentCache& cache{ *getCachePointer(entity) };
		cache.flags |= transform::componentFlags::position;
		cache.position = position;
	}

	void entityScript::setScale(const gameEntity::entity *const entity, math::v3 scale)
	{
		transform::componentCache& cache{ *getCachePointer(entity) };
		cache.flags |= transform::componentFlags::scale;
		cache.scale = scale;
	}
}

#ifdef USE_WITH_EDITOR

#include <atlsafe.h>

extern "C" __declspec(dllexport)
LPSAFEARRAY getScriptNames()
{
	const u32 size{ (u32)mooncastle::script::scriptNames().size() };
	if (!size) return nullptr;

	CComSafeArray<BSTR> names(size);

	for (u32 i{ 0 }; i < size; ++i)
	{
		names.SetAt(i, A2BSTR_EX(mooncastle::script::scriptNames()[i].c_str()), false);
	}
	return names.Detach();
}
#endif