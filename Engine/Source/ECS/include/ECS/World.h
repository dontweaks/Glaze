#pragma once

#include "Bundle/Bundle.h"
#include "Component/Component.h"

#include "Entity.h"

namespace glaze::ecs {
	struct World {
		Entity create_entity() {
			const auto entity = m_entity_manager.create_entity();
			//TODO handle archetypes
			return entity;
		}

		template<Bundle B>
		Entity create_entity(B&& bundle) {
			const auto entity = m_entity_manager.create_entity();
			//TODO handle archetypes
			return entity;
		}

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		Entity create_entity(Cs&& ... cs) {
			return create_entity(ComponentBundle{ std::forward_as_tuple(std::forward<Cs>(cs)...) });
		}

		bool destroy_entity(const Entity entity) noexcept {
			//TODO handle archetypes
			return m_entity_manager.destroy_entity(entity);
		}

		template<Bundle B>
		void add_bundle(const Entity entity, B&& bundle);

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		void add_components(const Entity entity, Cs&& ... cs) {
			return add_bundle(entity, ComponentBundle{ std::forward_as_tuple(std::forward<Cs>(cs)...) });
		}

		template<Bundle B>
		bool remove_bundle(const Entity entity) noexcept;

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		bool remove_components(const Entity entity) noexcept {
			return remove_bundle<ComponentBundle<Cs...>>(entity);
		}

		template<Bundle B>
		void register_bundle() {
			visit_bundle_types<B>([this]<Component C>() {
				m_component_manager.register_component<C>();
			});
		}

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		void register_components() {
			register_bundle<ComponentBundle<Cs...>>();
		}

		[[nodiscard]] WorldId world_id() const noexcept { return m_id; }

		[[nodiscard]] auto& entity_manager(this auto& self) noexcept { return self.m_entity_manager; }
		[[nodiscard]] auto& component_manager(this auto& self) noexcept { return self.m_component_manager; }

	private:
		WorldId m_id{0};

		EntityManager m_entity_manager;
		ComponentManager m_component_manager;
	};
}