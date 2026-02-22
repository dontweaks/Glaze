#pragma once

#include "Utils/Panic.h"
#include "ECS/Component/ComponentSignature.h"

#include "Table.h"

namespace glaze::ecs {
	struct TableManager {
		TableManager() {
			//insert empty table for entities without components
			m_tables.emplace_back(EMPTY_TABLE_ID);
		}

		std::pair<TableId, bool> try_emplace(const std::span<const ComponentId> table_components) {
			if (table_components.empty()) {
				return { EMPTY_TABLE_ID, false };
			}

			const ComponentSignatureView table_key{table_components, {}};
			const auto it = m_by_components.find(table_key);
			if (it != m_by_components.end()) {
				return { it->second, false };
			}

			const auto table_id = TableId::from_index(m_tables.size());
			m_by_components.emplace(table_key, table_id);
			m_tables.emplace_back(table_id);
			return { table_id, true };
		}

		[[nodiscard]] TableRow add_entity(const TableId table_id, const Entity entity) {
			const auto index = table_id.to_index();
			if (index >= m_tables.size()) {
				return utils::null_id;
			}

			return m_tables[index].add_entity(entity);
		}

		[[nodiscard]] std::span<const Table> tables() const noexcept { return m_tables; }
		[[nodiscard]] auto& empty_table(this auto& self) noexcept { return self.m_tables[EMPTY_TABLE_ID.to_index()]; }

		[[nodiscard]] auto& operator[](this auto& self, const TableId id) noexcept {
			return self.m_tables[id.to_index()];
		}

		[[nodiscard]] auto& at(this auto& self, const TableId id) noexcept {
			const auto index = id.to_index();
			if (index >= self.m_tables.size()) {
				utils::panic("Table id {} hasn't been registered yet", id.get());
			}

			return self.m_tables.at(index);
		}

		[[nodiscard]] size_t size() const noexcept { return m_tables.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_tables.empty(); }

	private:
		std::vector<Table> m_tables;
		ByComponentsMap<TableId> m_by_components;
	};
}