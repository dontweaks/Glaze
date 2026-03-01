#pragma once

#include "ECS/Storage/TypeErasedArray.h"
#include "ECS/Component/ComponentMeta.h"

namespace glaze::ecs {
	struct ComponentSparseSet {
		ComponentSparseSet(const ComponentMeta& component, const size_t capacity)
			: m_components(component.layout(), component.type_ops(), capacity), m_entities(capacity) {
		}

		ComponentSparseSet(const ComponentSparseSet& component) = delete;
		ComponentSparseSet& operator=(const ComponentSparseSet& component) = delete;

		ComponentSparseSet(ComponentSparseSet&& component) = default;
		ComponentSparseSet& operator=(ComponentSparseSet&& component) = default;

		template<Component T>
		void insert(const Entity entity, T&& data) {
			insert_untyped(entity, std::addressof(data));
		}

		template<Component T>
		[[nodiscard]] std::optional<std::reference_wrapper<T>> get(const Entity entity) noexcept {
			using U = std::remove_cvref_t<T>;
			return m_entities.at(entity.index()).transform([this](const auto table_row_ref) {
				const auto dense_index = table_row_ref.get();
				return *m_components.get<U>(dense_index.to_index());
			});
		}

		template<Component T>
		[[nodiscard]] std::optional<std::reference_wrapper<const T>> get(const Entity entity) const noexcept {
			using U = std::remove_cvref_t<T>;
			return m_entities.at(entity.index()).transform([this](const auto table_row_ref) {
				const auto dense_index = table_row_ref.get();
				return *m_components.get<U>(dense_index.to_index());
			});
		}

		template<Component T>
		[[nodiscard]] std::optional<T> swap_remove_and_destroy(const Entity entity) noexcept {
			using U = std::remove_cvref_t<T>;

			return m_entities.remove(entity.index()).transform([this](const TableRow table_row) {
				const size_t dense_index = table_row.to_index();
				const bool is_last = dense_index == m_components.size() - 1;

				if (!is_last) {
					const EntityIndex moved_entity = m_entities.indices()[dense_index];
					m_entities[moved_entity] = table_row;
				}

				return m_components.swap_remove<U>(dense_index);
			});
		}

		void insert_untyped(const Entity entity, void* const data) {
			if (const auto dense_index_opt = m_entities.at(entity.index())) {
				const auto dense_index = dense_index_opt.value().get();
				m_components.move_replace(dense_index.to_index(), data);
			} else {
				const auto table_row = TableRow::from_index(m_components.size());
				m_components.move_emplace_back(data);
				m_entities.insert(entity.index(), table_row);
			}
		}

		[[nodiscard]] std::optional<void*> get_untyped(const Entity entity) noexcept {
			return m_entities.at(entity.index()).transform([this](const auto dense_index_ref) {
				const auto dense_index = dense_index_ref.get();
				return m_components.get(dense_index.to_index());
			});
		}

		[[nodiscard]] std::optional<const void*> get_untyped(const Entity entity) const noexcept {
			return m_entities.at(entity.index()).transform([this](const auto dense_index_ref) {
				const auto dense_index = dense_index_ref.get();
				return m_components.get(dense_index.to_index());
			});
		}

		void remove_and_destroy_untyped(const Entity entity) noexcept {
			const auto table_row = m_entities.remove(entity.index());
			if (!table_row) {
				return;
			}

			const size_t dense_index = table_row->to_index();
			const bool is_last = dense_index == m_components.size() - 1;

			m_components.swap_remove(dense_index);

			if (!is_last) {
				const EntityIndex moved_entity = m_entities.indices()[dense_index];
				m_entities[moved_entity] = *table_row;
			}
		}

		[[nodiscard]] size_t size() const noexcept { return m_components.size(); }
		[[nodiscard]] size_t capacity() const noexcept { return m_components.capacity(); }
		[[nodiscard]] bool empty() const noexcept { return m_components.empty(); }
		[[nodiscard]] bool contains(const Entity entity) const noexcept { return m_entities.contains(entity.index()); }

	private:
		TypeErasedArray m_components;
		SparseSet<EntityIndex, TableRow> m_entities;
	};
}
