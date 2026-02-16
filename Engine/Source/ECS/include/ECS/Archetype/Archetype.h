#pragma once

#include <filesystem>
#include <vector>
#include <ranges>
#include <span>
#include <utility>

#include "ECS/Entity.h"
#include "ECS/Bundle/Bundle.h"
#include "ECS/Storage/Table.h"
#include "ECS/Storage/SparseSet.h"
#include "ECS/Component/Component.h"

namespace glaze::ecs {
	struct ArchetypeEntity {
		Entity entity;
		TableRow table_row;
	};

	struct ArchetypeRecord {
		size_t column;
	};
	using ArchetypeRecordMap = std::unordered_map<ArchetypeId, ArchetypeRecord>;
	using ComponentIndex = std::unordered_map<ComponentId, ArchetypeRecordMap>;

	struct ArchetypeEdge {
		ArchetypeId add;
		ArchetypeId remove;
		ArchetypeId take;
	};

	struct Archetype {
		Archetype(const ArchetypeId id,
			const TableId table_id,
			ComponentIndex& component_index,
			const std::span<const ComponentId> table_components,
			const std::span<const ComponentId> sparse_components)
			: m_id(id), m_table_id(table_id)
		{
			const size_t component_count = table_components.size() + sparse_components.size();
			m_components.reserve(component_count);

			for (const auto [i, c_id] : table_components | std::views::enumerate) {
				m_components.insert(c_id, StorageType::Table);
				component_index[c_id][id] = ArchetypeRecord {
					.column = static_cast<size_t>(i)
				};
			}

			for (const ComponentId c_id : sparse_components) {
				m_components.insert(c_id, StorageType::SparseSet);
				component_index[c_id][id] = ArchetypeRecord{
					//TODO: handle -1 case
					.column = -1u
				};
			}
		}

		Archetype(const Archetype& other) = delete;
		Archetype& operator=(const Archetype& other) = delete;

		Archetype(Archetype&& other) noexcept = default;
		Archetype& operator=(Archetype&& other) noexcept = default;

		EntityLocation add_entity(const Entity entity, const TableRow table_row) {
			const ArchetypeRow archetype_row{static_cast<uint32_t>(entity_count())};
			m_entities.emplace_back(entity, table_row);

			return EntityLocation {
				.archetype_id = m_id,
				.archetype_row = archetype_row,
				.table_id = m_table_id,
				.table_row = table_row
			};
		}

		[[nodiscard]] TableRow entity_table_row(const ArchetypeRow row) const noexcept {
			return m_entities[row.to_index()].table_row;
		}

		void set_entity_table_row(const ArchetypeRow row, const TableRow table_row) noexcept {
			m_entities[row.to_index()].table_row = table_row;
		}

		[[nodiscard]] std::span<const ArchetypeEntity> entities() const noexcept { return m_entities; }

		[[nodiscard]] auto components() const noexcept { return m_components.indices(); }

		[[nodiscard]] auto table_components() const noexcept {
			return m_components.iter()
				| std::views::filter([](const auto& p) { return std::get<1>(p) == StorageType::Table; })
				| std::views::elements<0>;
		}

		[[nodiscard]] auto sparse_components() const noexcept {
			return m_components.iter()
				| std::views::filter([](const auto& p) { return std::get<1>(p) == StorageType::SparseSet; })
				| std::views::elements<0>;
		}

		[[nodiscard]] std::optional<StorageType> get_component_storage_type(const ComponentId component_id) const noexcept {
			return m_components.at(component_id).transform([](const auto storage_ref) { return storage_ref.get(); });
		}

		[[nodiscard]] auto& edges(this auto& self) noexcept { return self.m_edges; }

		[[nodiscard]] ArchetypeId id() const noexcept { return m_id; }
		[[nodiscard]] TableId table_id() const noexcept { return m_table_id; }

		[[nodiscard]] size_t entity_count() const noexcept { return m_entities.size(); }
		[[nodiscard]] size_t component_count() const noexcept { return m_components.size(); }

		[[nodiscard]] bool empty() const noexcept { return m_entities.empty(); }
		[[nodiscard]] bool has_component(const ComponentId component_id) const noexcept { return m_components.contains(component_id); }

		void clear_entities() noexcept { m_entities.clear(); }

	private:
		ArchetypeId m_id;
		TableId m_table_id;

		std::vector<ArchetypeEntity> m_entities;

		SparseArray<BundleId, ArchetypeEdge> m_edges;
		SparseSet<ComponentId, StorageType> m_components;
	};

	using ArchetypeByComponents = std::unordered_map<
		ComponentSignature,
		ArchetypeId,
		ComponentSignatureHasher,
		ComponentSignatureEq
	>;

	struct ArchetypeManager {
		ArchetypeManager() {
			//insert empty archetype for entities without components
			try_emplace(EMPTY_TABLE_ID, {}, {});
		}

		ArchetypeManager(const ArchetypeManager& other) = delete;
		ArchetypeManager& operator=(const ArchetypeManager& other) = delete;

		ArchetypeManager(ArchetypeManager&& other) = delete;
		ArchetypeManager& operator=(ArchetypeManager&& other) = delete;

		std::pair<ArchetypeId, bool> try_emplace(const TableId table_id,
			const std::span<const ComponentId> table_components,
			const std::span<const ComponentId> sparse_components) {
			const ComponentSignatureView archetype_key{table_components, sparse_components};

			const auto it = m_by_components.find(archetype_key);
			if (it != m_by_components.end()) {
				return { it->second, false };
			}

			const auto archetype_id = ArchetypeId::from_index(m_archetypes.size());
			m_by_components.emplace(archetype_key, archetype_id);
			m_archetypes.emplace_back(archetype_id, table_id, m_component_index, table_components, sparse_components);
			return { archetype_id, true };
		}

		[[nodiscard]] std::pair<ArchetypeId, bool> add_bundle_to_archetype(
			const ArchetypeId archetype_id,
			const BundleMeta& bundle_meta,
			TableManager& table_manager) {
			const BundleId bundle_id = bundle_meta.id();

			{
				auto& archetype = m_archetypes[archetype_id.to_index()];
				auto& edges = archetype.edges();

				if (const auto add_edge_opt = edges.at(bundle_id)) {
					if (const auto add_edge = add_edge_opt.value().get().add; add_edge.valid()) {
						return { edges[bundle_id].add, false };
					}
				}
			}

			std::vector<ComponentId> new_table_components;
			new_table_components.reserve(bundle_meta.table_components_count());
			std::vector<ComponentId> new_sparse_components;
			new_sparse_components.reserve(bundle_meta.sparse_components_count());

			{
				auto& archetype = m_archetypes[archetype_id.to_index()];
				for (const auto component_id : bundle_meta.table_components()) {
					if (archetype.has_component(component_id)) {
						//TODO: component exists, we should update/replace or remove
					} else {
						new_table_components.push_back(component_id);
					}
				}

				for (const auto component_id : bundle_meta.sparse_components()) {
					if (archetype.has_component(component_id)) {
						//TODO: component exists, we should update/replace or remove
					} else {
						new_sparse_components.push_back(component_id);
					}
				}
			}

			//no new components means no archetype change
			if (new_table_components.empty() && new_sparse_components.empty()) {
				auto& archetype = m_archetypes[archetype_id.to_index()];
				auto& edges = archetype.edges();
				edges.insert(bundle_id, ArchetypeEdge{.add = archetype_id});
				return {archetype_id, false};
			}

			std::ranges::sort(new_table_components);
			std::ranges::sort(new_sparse_components);

			//we've got some new components, combine them with old ones and create a new archetype
			{
				const auto& archetype = m_archetypes[archetype_id.to_index()];
				std::vector<ComponentId> merged;

				merged.reserve(new_table_components.size() + archetype.component_count());
				std::ranges::set_union(new_table_components, archetype.table_components(), std::back_inserter(merged));
				new_table_components = std::move(merged);

				merged.clear();
				merged.reserve(new_sparse_components.size() + archetype.component_count());
				std::ranges::set_union(new_sparse_components, archetype.sparse_components(), std::back_inserter(merged));
				new_sparse_components = std::move(merged);
			}

			const auto table_id = table_manager.emplace(new_table_components);

			const auto [new_archetype_id, is_new_created] = try_emplace(table_id, new_table_components, new_sparse_components);
			{
				auto& archetype = m_archetypes[archetype_id.to_index()];
				auto& edges = archetype.edges();
				edges.insert(bundle_id, ArchetypeEdge{.add = new_archetype_id});
			}
			return { new_archetype_id, is_new_created };
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
		std::vector<Archetype> m_archetypes;
		ArchetypeByComponents m_by_components;
		ComponentIndex m_component_index;
	};
}
