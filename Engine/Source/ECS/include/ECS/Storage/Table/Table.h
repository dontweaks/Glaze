#pragma once

#include "ECS/Entity.h"
#include "ECS/Storage/TypeErasedArray.h"
#include "ECs/Storage/SparseSet/SparseSet.h"

namespace glaze::ecs {
	struct Table {
		explicit Table(const TableId id) noexcept
			: m_id(id) {
		}

		void add_column(const ComponentMeta& component_meta) {
			m_columns.emplace(component_meta.id(), component_meta.layout(), component_meta.type_ops());
		}

		[[nodiscard]] TableRow add_entity(const Entity entity) {
			m_entities.push_back(entity);
			return TableRow::from_index(m_entities.size() - 1);
		}

		[[nodiscard]] TableId id() const noexcept { return m_id; }

		[[nodiscard]] auto& operator[](this auto& self, const ComponentId id) noexcept {
			return self.m_columns[id];
		}

		[[nodiscard]] auto at(const ComponentId id) noexcept {
			return m_columns.at(id);
		}

		[[nodiscard]] auto at(const ComponentId id) const noexcept {
			return m_columns.at(id);
		}

	private:
		std::vector<Entity> m_entities;
		SparseSet<ComponentId, TypeErasedArray> m_columns;
		TableId m_id;
	};
}