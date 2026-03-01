#pragma once

#include "ECS/Entity.h"
#include "ECS/Component/ComponentMeta.h"

namespace glaze::ecs {
	struct Table {
		explicit Table(const TableId id) noexcept
			: m_id(id) {
		}

		void add_column(const ComponentMeta& component_meta) {

		}

		[[nodiscard]] TableRow add_entity(const Entity entity) {
			m_entities.push_back(entity);
			return TableRow::from_index(m_entities.size() - 1);
		}

		[[nodiscard]] TableId id() const noexcept { return m_id; }

	private:
		std::vector<Entity> m_entities;
		TableId m_id;
	};
}