#pragma once

#include "Bundle/BundleManager.h"
#include "Component/ComponentManager.h"
#include "Archetype/ArchetypeManager.h"
#include "Storage/Storage.h"

#include "Entity.h"

namespace glaze::ecs {
	struct World {
		Entity create_entity() {
			const auto entity = m_entity_manager.create_entity();

			auto& table = m_storage.empty_table();
			const auto table_row = table.add_entity(entity);

			auto& archetype = m_archetype_manager.empty_archetype();
			const auto location = archetype.add_entity(entity, table_row);

			m_entity_manager.set_location(entity, location);
			return entity;
		}

		template<Bundle B>
		Entity create_entity(B&& bundle) {
			const auto entity = m_entity_manager.create_entity();

			const auto bundle_id = register_bundle<B>();
			const auto [archetype_id, table_id] = m_archetype_manager.add_bundle_to_archetype(
				EMPTY_ARCHETYPE_ID,
				bundle_id,
				m_bundle_manager,
				m_component_manager,
				m_storage.table_manager);

			auto& archetype = m_archetype_manager[archetype_id];
			auto& table = m_storage[table_id];

			const auto table_row = table.add_entity(entity);
			const auto location = archetype.add_entity(entity, table_row);

			m_entity_manager.set_location(entity, location);

			m_storage.write_bundle(std::forward<B>(bundle), entity, m_bundle_manager[bundle_id]);

			return entity;
		}

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		Entity create_entity(Cs&& ... cs) {
			return create_entity(ComponentBundle{ std::forward<Cs>(cs)... });
		}

		bool destroy_entity(const Entity entity) noexcept {
			//TODO handle archetypes
			return m_entity_manager.destroy_entity(entity);
		}

		template<Bundle B>
		void add_bundle(const Entity entity, B&& bundle);

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		void add_components(const Entity entity, Cs&& ... cs) {
			return add_bundle(entity, ComponentBundle{ std::forward<Cs>(cs)... });
		}

		template<Bundle B>
		bool remove_bundle(const Entity entity) noexcept;

		template<Component ... Cs> requires (sizeof ... (Cs) > 0)
		bool remove_components(const Entity entity) noexcept {
			return remove_bundle<ComponentBundle<Cs...>>(entity);
		}

		template<Bundle B>
		BundleId register_bundle() {
			return m_bundle_manager.register_bundle<B>(m_component_manager, m_storage);
		}

		template<Component C>
		ComponentId register_component() {
			return m_component_manager.register_component<C>();
		}

		template<Component ... Cs> requires (sizeof ... (Cs) > 1)
		std::array<ComponentId, sizeof ... (Cs)> register_components() {
			return { register_component<Cs>()... };
		}

		[[nodiscard]] WorldId world_id() const noexcept { return m_id; }

		[[nodiscard]] auto& entity_manager(this auto& self) noexcept { return self.m_entity_manager; }
		[[nodiscard]] auto& component_manager(this auto& self) noexcept { return self.m_component_manager; }
		[[nodiscard]] auto& archetype_manager(this auto& self) noexcept { return self.m_archetype_manager; }
		[[nodiscard]] auto& bundle_manager(this auto& self) noexcept { return self.m_bundle_manager; }
		[[nodiscard]] auto& storage(this auto& self) noexcept { return self.m_storage; }

	private:
		WorldId m_id{0};

		EntityManager m_entity_manager;
		ComponentManager m_component_manager;
		ArchetypeManager m_archetype_manager;
		BundleManager m_bundle_manager;
		Storage m_storage;
	};
}