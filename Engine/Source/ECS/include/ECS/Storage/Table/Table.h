#pragma once

#include "ECS/Entity.h"
#include "ECS/Storage/TypeErasedArray.h"
#include "ECS/Storage/SparseSet/SparseSet.h"

namespace glaze::ecs {
	struct Table {
		explicit Table(const TableId id) noexcept
			: m_id(id) {
		}

		Table(const Table& other) = delete;
		Table& operator=(const Table& other) = delete;

		Table(Table&& other) noexcept = default;
		Table& operator=(Table&& other) noexcept = default;

		void add_column(const ComponentMeta& component_meta) {
			m_columns.emplace(component_meta.id(), component_meta.layout(), component_meta.type_ops());
		}

		//returns a valid entity that has been put into the removed entity place and a table row
		//nullopt if last because there's no valid entity then
		[[nodiscard]] std::pair<std::optional<Entity>, TableRow> move_to(Table& dst, const TableRow table_row) {
			const auto index = table_row.to_index();
			assert(index < entity_count());
			const bool is_last = index == entity_count() - 1;

			const auto new_table_row = dst.add_entity(utils::swap_remove(m_entities, index));
			for (const auto& [component_id, src_column] : m_columns.iter()) {
				auto& new_column = utils::value_or_panic(dst.at(component_id));
				new_column.move_insert(new_table_row.to_index(), src_column.get(index));
				src_column.swap_remove(index);
			}

			return { is_last ? std::nullopt : std::make_optional(m_entities[index]), new_table_row };
		}

		[[nodiscard]] TableRow add_entity(const Entity entity) {
			m_entities.push_back(entity);
			return TableRow::from_index(m_entities.size() - 1);
		}

		[[nodiscard]] std::optional<Entity> remove_entity(const TableRow row) noexcept {
			for (auto& column : m_columns.values()) {
				column.swap_remove(row.to_index());
			}

			const bool is_last = row.to_index() == m_entities.size() - 1;
			utils::swap_remove(m_entities, row.to_index());

			if (is_last) {
				return std::nullopt;
			}

			return m_entities[row.to_index()];
		}

		[[nodiscard]] TableId id() const noexcept { return m_id; }

		[[nodiscard]] auto& operator[](this auto& self, const ComponentId id) noexcept {
			return self.m_columns[id];
		}

		[[nodiscard]] utils::optional_ref<TypeErasedArray> at(const ComponentId id) noexcept {
			return m_columns.at(id);
		}

		[[nodiscard]] utils::optional_ref<const TypeErasedArray> at(const ComponentId id) const noexcept {
			return m_columns.at(id);
		}

		[[nodiscard]] size_t entity_count() const noexcept { return m_entities.size(); }
		[[nodiscard]] size_t component_count() const noexcept { return m_columns.size(); }

	private:
		std::vector<Entity> m_entities;
		SparseSet<ComponentId, TypeErasedArray> m_columns;
		TableId m_id;
	};
}
