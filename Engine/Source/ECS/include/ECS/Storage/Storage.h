#pragma once

#include "Table/TableManager.h"
#include "ECS/Bundle/BundleMeta.h"
#include "SparseSet/ComponentSparseSet.h"

namespace glaze::ecs {
	struct Storage {
		void ensure_component(const ComponentMeta& component) {
			if (component.storage_type() == StorageType::SparseSet) {
				if (!sparse_sets.contains(component.id())) {
					sparse_sets.emplace(component.id(), component, 64);
				}
			}
		}

		template<Bundle B>
		void write_bundle(B&& bundle, const Entity entity, const EntityLocation& location, const BundleMeta& bundle_meta) {
			visit_bundle_enumerate(std::forward<B>(bundle), [&]<Component C>(const size_t index, C&& c) {
				using T = std::remove_cvref_t<C>;
				const auto component_id = bundle_meta.components()[index];

				if (get_storage_type<T>() == StorageType::Table) {
					auto& table = table_manager.at(location.table_id);
					const auto column_opt = table.at(component_id);
					if (!column_opt.has_value()) {
						utils::panic("Trying to write bundle to non existing column");
					}
					auto& column = column_opt->get();
					column.insert(location.table_row.to_index(), std::forward_like<C>(c));
				} else {
					const auto sparse_set_opt = sparse_sets.at(component_id);
					if (!sparse_set_opt.has_value()) {
						utils::panic("Trying to write bundle to non existing sparse set");
					}

					auto& sparse_set = sparse_set_opt->get();
					sparse_set.insert(entity, std::forward_like<C>(c));
				}
			});
		}

		[[nodiscard]] auto& operator[](this auto& self, const TableId id) noexcept {
			return self.table_manager[id];
		}

		[[nodiscard]] auto& operator[](this auto& self, const ComponentId id) noexcept {
			return self.sparse_sets[id];
		}

		[[nodiscard]] auto& empty_table(this auto& self) noexcept { return self.table_manager.empty_table(); }

		SparseSet<ComponentId, ComponentSparseSet> sparse_sets;
		TableManager table_manager;
	};
}