 #pragma once

#include "../Components/ComponentsCommon.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"
#include "GeometryComponent.h"

#include <string>

namespace mooncastle::gameEntity
{
	DEFINE_TYPED_ID(entityId);

	class entity
	{
	public:
		constexpr explicit entity(entityId id) : id{ id } {}
		constexpr entity() : id{ id::invalidId } {}
		[[nodiscard]] constexpr entityId getId() const { return id; }
		[[nodiscard]] constexpr bool isValid() const { return id::isValid(id); }

		[[nodiscard]] transform::component transform() const;
		[[nodiscard]] script::component script() const;
		[[nodiscard]] geometry::component geometry() const;

		[[nodiscard]] math::v4 rotation() const { return transform().rotation(); }
		[[nodiscard]] math::v3 orientation() const { return transform().orientation(); }
		[[nodiscard]] math::v3 position() const { return transform().position(); }
		[[nodiscard]] math::v3 scale() const { return transform().scale(); }

	private:
		entityId id;
	};
}

namespace mooncastle::script
{
	class entityScript : public gameEntity::entity
	{
	public:
		virtual ~entityScript() = default;
		virtual void beginPlay() {}
		virtual void update(f32) {}

	protected:
		constexpr explicit entityScript(gameEntity::entity entity) : gameEntity::entity{ entity.getId() } {}

		void setRotation(math::v4 rotationQuaternion) const { setRotation(this, rotationQuaternion); }
		void setOrientation(math::v3 orientationVector) const { setOrientation(this, orientationVector); }
		void setPosition(math::v3 position) const { setPosition(this, position); }
		void setScale(math::v3 scale) const { setScale(this, scale); }

		static void setRotation(const gameEntity::entity *const entity, math::v4 rotationQuaternion);
		static void setOrientation(const gameEntity::entity *const entity, math::v3 orientationVector);
		static void setPosition(const gameEntity::entity *const entity, math::v3 position);
		static void setScale(const gameEntity::entity *const entity, math::v3 scale);
	};

	namespace detail
	{
		using script_ptr = std::unique_ptr<entityScript>;
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