#pragma once

#include "Archetype.h"

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
			m_archetypes.emplace_back(archetype_id, table_id, m_component_index, archetype_key);
			return { archetype_id, true };
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
		ByComponentsMap<ArchetypeId> m_by_components;
		ComponentIndex m_component_index;
	};
}
