#pragma once

#include "Archetype.h"
#include "ECS/Storage/Table/TableManager.h"

namespace glaze::ecs {
	struct ArchetypeManager {
		ArchetypeManager() {
			//insert empty archetype for entities without components
			try_emplace(EMPTY_TABLE_ID, {}, {});
		}

		ArchetypeManager(const ArchetypeManager& other) = delete;
		ArchetypeManager& operator=(const ArchetypeManager& other) = delete;

		ArchetypeManager(ArchetypeManager&& other) = delete;
		ArchetypeManager& operator=(ArchetypeManager&& other) = delete;

		[[nodiscard]] std::pair<ArchetypeId, TableId> add_bundle_to_archetype(
			const ArchetypeId source_archetype_id,
			const BundleId bundle_id,
			const BundleManager& bundle_manager,
			const ComponentManager& component_manager,
			TableManager& table_manager
		) {
			auto& archetype = m_archetypes[source_archetype_id.to_index()];
			auto& edges = archetype.edges();

			//return early if we have cached edge
			if (const auto edge = edges.at(bundle_id)) {
				const auto add_edge = edge.value().get().add;
				if (add_edge.valid()) {
					return { add_edge, m_archetypes[add_edge.to_index()].table_id() };
				}
			}

			const auto& bundle = bundle_manager[bundle_id];

			std::vector<ComponentId> new_table_components;
			new_table_components.reserve(bundle.table_components_count());

			std::vector<ComponentId> new_sparse_components;
			new_sparse_components.reserve(bundle.sparse_components_count());

			for (const auto component_id : bundle.table_components()) {
				if (archetype.has_component(component_id)) {
					//TODO: component exists, we should update/replace or remove
				} else {
					new_table_components.push_back(component_id);
				}
			}
			for (const auto component_id : bundle.sparse_components()) {
				if (archetype.has_component(component_id)) {
					//TODO: component exists, we should update/replace or remove
				} else {
					new_sparse_components.push_back(component_id);
				}
			}

			//no new components means no archetype change
			if (new_table_components.empty() && new_sparse_components.empty()) {
				edges.insert(bundle_id, ArchetypeEdge{.add = source_archetype_id});
				return { source_archetype_id, archetype.table_id() };
			}

			//we've got some new components, combine them with old ones, sort and create a new archetype
			new_table_components.append_range(archetype.table_components());
			new_sparse_components.append_range(archetype.sparse_components());
			std::ranges::sort(new_table_components);
			std::ranges::sort(new_sparse_components);

			const auto table_id = table_manager.try_emplace(new_table_components, component_manager);
			const auto new_archetype_id = try_emplace(table_id, new_table_components, new_sparse_components);

			//a new archetype was created so existing archetype ref and edges ref are invalid now, get new ones and cache the edge
			auto& valid_archetype = m_archetypes[source_archetype_id.to_index()];
			auto& valid_edges = valid_archetype.edges();
			valid_edges.insert(bundle_id, ArchetypeEdge{.add = new_archetype_id});

			return { new_archetype_id, table_id };
		}

		[[nodiscard]] ArchetypeVersion version() const noexcept { return ArchetypeVersion::from_index(m_archetypes.size()); }

		[[nodiscard]] const ComponentIndex& component_index() const noexcept { return m_component_index; }
		[[nodiscard]] std::span<const Archetype> archetypes() const noexcept { return m_archetypes; }

		[[nodiscard]] auto& empty_archetype(this auto& self) noexcept { return self.m_archetypes[EMPTY_ARCHETYPE_ID.to_index()]; }

		[[nodiscard]] auto& operator[](this auto& self, const ArchetypeId id) noexcept { return self.m_archetypes[id.to_index()]; }

		[[nodiscard]] auto& at(this auto& self, const ArchetypeId id) noexcept {
			const auto index = id.to_index();
			if (index >= self.m_archetypes.size()) {
				utils::panic("Archetype id {} is out of range", id.get());
			}

			return self.m_archetypes.at(index);
		}

		[[nodiscard]] size_t size() const noexcept { return m_archetypes.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_archetypes.empty(); }

		void clear_entities() noexcept {
			for (auto& archetype : m_archetypes) {
				archetype.clear_entities();
			}
		}

	private:
		ArchetypeId try_emplace(const TableId table_id,
			const std::span<const ComponentId> table_components,
			const std::span<const ComponentId> sparse_components) {
			const ComponentSignatureView archetype_key{table_components, sparse_components};

			const auto it = m_by_components.find(archetype_key);
			if (it != m_by_components.end()) {
				return it->second;
			}

			const auto archetype_id = ArchetypeId::from_index(m_archetypes.size());
			m_by_components.emplace(archetype_key, archetype_id);
			m_archetypes.emplace_back(archetype_id, table_id, m_component_index, archetype_key);
			return archetype_id;
		}

		std::vector<Archetype> m_archetypes;
		ByComponentsMap<ArchetypeId> m_by_components;
		ComponentIndex m_component_index;
	};
}
