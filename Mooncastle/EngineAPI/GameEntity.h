 #pragma once

#include "Components/ComponentsCommon.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"

#include <string>

namespace mooncastle
{
	namespace gameEntity
	{
		DEFINE_TYPED_ID(entityId);

		class entity
		{
		public:
			constexpr explicit entity(entityId id) : id{ id } {}
			constexpr entity() : id{ id::invalidId } {}
			constexpr entityId getId() const { return id; }
			constexpr bool isValid() const { return id::isValid(id); }

			transform::component transform() const;
			script::component script() const;
		private:
			entityId id;
		};
	}

	namespace script 
	{
		class entity_script : public gameEntity::entity
		{
		public:
			virtual ~entity_script() = default;
			virtual void beginPlay() {}
			virtual void update(float deltaTime) {}
		protected:
			constexpr explicit entity_script(gameEntity::entity entity) : gameEntity::entity{ entity.getId()} {}
		};

		namespace detail
		{
			using script_ptr = std::unique_ptr<entity_script>;
			using script_creator = script_ptr(*)(gameEntity::entity entity);
			using string_hash = std::hash<std::string>;

			u8 registerScript(size_t, script_creator);

			#ifdef USE_WITH_EDITOR
			extern "C" __declspec(dllexport)
			#endif
			script_creator getScriptCreator(size_t tag);

			template<class script_class>

			script_ptr create_script(gameEntity::entity entity)
			{
				assert(entity.isValid());
				return std::make_unique<script_class>(entity);
			}

#ifdef USE_WITH_EDITOR
u8 addScriptName(const char* name);

#define REGISTER_SCRIPT(TYPE)                                        \
		namespace {                                                  \
			static u8                                                \
			_reg##TYPE =                                             \
			{ mooncastle::script::detail::registerScript(            \
			mooncastle::script::detail::string_hash()(#TYPE),        \
			&mooncastle::script::detail::create_script<TYPE>) };     \
			const u8 _name_##TYPE                                  \
			{ mooncastle::script::detail::addScriptName(#TYPE) };     \
			}
#else
#define REGISTER_SCRIPT(TYPE)                                        \
		namespace {                                                  \
			static u8                                                \
			_reg##TYPE =                                             \
			{ mooncastle::script::detail::registerScript(            \
			mooncastle::script::detail::string_hash()(#TYPE),        \
			&mooncastle::script::detail::create_script<TYPE>) };     \
			}
#endif
		}
	}
}