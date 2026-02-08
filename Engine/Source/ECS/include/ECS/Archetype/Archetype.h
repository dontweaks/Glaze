#pragma once

#include <vector>
#include <ranges>
#include <span>
#include <utility>

#include "ECS/Entity.h"
#include "ECS/Component/Component.h"

#include "Utils/HashCombine.h"

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

	struct Archetype {
		Archetype(const ArchetypeId id,
			const TableId table_id,
			ComponentIndex& component_index,
			const std::span<const ComponentId> table_components,
			const std::span<const ComponentId> sparse_set_components)
			: m_id(id), m_table_id(table_id)
		{
			const size_t component_count = table_components.size() + sparse_set_components.size();
			m_component_storages.resize(component_count);

			for (const auto [i, c_id] : table_components | std::views::enumerate) {
				m_component_storages[c_id.to_index()] = StorageType::Table;
				component_index[c_id][id] = ArchetypeRecord {
					.column = static_cast<size_t>(i)
				};
			}

			for (const ComponentId c_id : sparse_set_components) {
				m_component_storages[c_id.to_index()] = StorageType::SparseSet;
				component_index[c_id][id] = ArchetypeRecord{
					.column = -1u
				};
			}
		}

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

		[[nodiscard]] auto components() const noexcept {
			return m_component_storages | std::views::enumerate | std::views::keys | std::views::transform(ComponentId::from_index);
		}

		[[nodiscard]] auto table_components() const noexcept {
			return m_component_storages | std::views::enumerate
				| std::views::filter([](const auto& pair) {
					const auto& [i, storage] = pair;
					return storage == StorageType::Table;
				})
				| std::views::keys
				| std::views::transform(ComponentId::from_index);
		}
		[[nodiscard]] auto sparse_set_components() const noexcept {
			return m_component_storages | std::views::enumerate
				| std::views::filter([](const auto& pair) {
					const auto& [i, storage] = pair;
					return storage == StorageType::SparseSet;
				})
				| std::views::keys
				| std::views::transform(ComponentId::from_index);
		}

		[[nodiscard]] const StorageType* get_component_storage_type(const ComponentId component_id) const noexcept {
			const auto index = component_id.to_index();
			if (index >= m_component_storages.size()) {
				return nullptr;
			}
			return &m_component_storages[index];
		}

		[[nodiscard]] ArchetypeId id() const noexcept { return m_id; }
		[[nodiscard]] TableId table_id() const noexcept { return m_table_id; }

		[[nodiscard]] size_t entity_count() const noexcept { return m_entities.size(); }
		[[nodiscard]] size_t component_count() const noexcept { return m_component_storages.size(); }

		[[nodiscard]] bool empty() const noexcept { return m_entities.empty(); }
		[[nodiscard]] bool has_component(const ComponentId component_id) const noexcept {
			return m_component_storages.size() > component_id.to_index();
		}

		void clear_entities() noexcept { m_entities.clear(); }

	private:
		ArchetypeId m_id;
		TableId m_table_id;

		std::vector<ArchetypeEntity> m_entities;

		//ComponentId is the index
		std::vector<StorageType> m_component_storages;
	};

	struct ArchetypeComponentsHasher {
		using Type = size_t;

		[[nodiscard]] size_t operator()(const std::span<const ComponentId>& table_components, const std::span<const ComponentId>& sparse_set_components) const noexcept {
			size_t h = 0;
			utils::hash_combine_with<ComponentIdHasher>(h, table_components, sparse_set_components);
			return h;
		}
	};
	using ArchetypeByComponents = std::unordered_map<ArchetypeComponentsHasher::Type, ArchetypeId>;

	struct ArchetypeManager {
		ArchetypeManager() {
			//insert empty archetype for entities without components
			try_emplace(EMPTY_TABLE_ID, {}, {});
		}

		std::pair<ArchetypeId, bool> try_emplace(const TableId table_id,
			const std::span<const ComponentId> table_components,
			const std::span<const ComponentId> sparse_set_components) {
			ArchetypeComponentsHasher::Type archetype_components = ArchetypeComponentsHasher{}(table_components, sparse_set_components);

			const auto it = m_by_components.find(archetype_components);
			if (it != m_by_components.end()) {
				return { it->second, false };
			}

			const auto archetype_id = ArchetypeId::from_index(m_archetypes.size());
			m_by_components.emplace(archetype_components, archetype_id);
			m_archetypes.emplace_back(archetype_id, table_id, m_component_index, table_components, sparse_set_components);
			return { archetype_id, true };
		}

		[[nodiscard]] ArchetypeVersion version() const noexcept {
			return ArchetypeVersion::from_index(m_archetypes.size());
		}

		[[nodiscard]] const ComponentIndex& component_index() const noexcept { return m_component_index; }

		[[nodiscard]] std::span<const Archetype> archetypes() const noexcept { return m_archetypes; }

		[[nodiscard]] auto& empty_archetype(this auto& self) noexcept {
			return self.m_archetypes[EMPTY_ARCHETYPE_ID.to_index()];
		}

		[[nodiscard]] auto& operator[](this auto& self, const ArchetypeId id) noexcept {
			return self.m_archetypes[id.to_index()];
		}

		[[nodiscard]] auto& at(this auto& self, const ArchetypeId id) {
			return self.m_archetypes.at(id.to_index());
		}

		void clear_entities() noexcept {
			for (auto& archetype : m_archetypes) {
				archetype.clear_entities();
			}
		}

		[[nodiscard]] size_t size() const noexcept {
			return m_archetypes.size();
		}

		[[nodiscard]] bool empty() const noexcept {
			return m_archetypes.empty();
		}

	private:
		std::vector<Archetype> m_archetypes;
		ArchetypeByComponents m_by_components;
		ComponentIndex m_component_index;
	};
}
