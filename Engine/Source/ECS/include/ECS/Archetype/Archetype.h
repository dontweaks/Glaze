#pragma once

#include <ranges>

#include "ECS/Entity.h"
#include "ECS/Storage/SparseSet.h"
#include "ECS/Component/Component.h"
#include "ECS/Component/ComponentSignature.h"

#include "ECS/Ids.h"

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
			const ComponentSignatureView component_signature)
			: m_id(id), m_table_id(table_id)
		{
			m_components.reserve(component_signature.component_count());

			for (const auto [i, c_id] : component_signature.table | std::views::enumerate) {
				m_components.insert(c_id, StorageType::Table);
				component_index[c_id][id] = ArchetypeRecord {
					.column = static_cast<size_t>(i)
				};
			}

			for (const ComponentId c_id : component_signature.sparse) {
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
}