#include "Script.h"
#include "Entity.h"

namespace mooncastle::script 
{
	namespace
	{
		utl::vector<detail::script_ptr>     entityScripts;
		utl::vector<id::idType>             idMapping;

		utl::vector<id::generationType>     generations;
		utl::vector <scriptId>              freeIds;

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

		bool exists(scriptId id) 
		{
			assert(id::isValid(id));

			const id::idType index{ id::index(id) };

			assert(index < generations.size() && idMapping[index] < entityScripts.size());
			assert(generations[index] == id::generation(id));

			return (generations[index] == id::generation(id)) && (entityScripts[idMapping[index]]) && (entityScripts[idMapping[index]])->isValid();
		};
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

			freeIds.pop_back();
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

		utl::eraseUnordered(entityScripts, index);

		idMapping[id::index(lastId)] = index;
		idMapping[id::index(id)] = id::invalidId;
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