#pragma once

#include <tuple>
#include <ranges>

#include "Utils/Panic.h"

#include "ECS/Component/Component.h"

/*
	Bundle is a set of components.

	They're used for improve performance of adding a few components at once.
	eg.
	We create an entity and we want to add 3 Components - let's say Position, Velocity, Render.

	Doing it 1 by 1 we'd need to create 3 archetypes and move data between them.

	add_component<Position>(entity) -> creates Archetype<Position>
	add_component<Velocity>(entity) -> creates Archetype<Position, Velocity>         -> moves data from Archetype<Position>
	add_component<Render>(entity)   -> creates Archetype<Position, Velocity, Render> -> moves data from Archetype<Position, Velocity>

	With a bundle we can do all this hard work at once

	add_bundle<MyBundle<Position, Velocity, Render>> -> creates Archetype<Position, Velocity, Render>

	Same thing applies to removing components
 */

namespace glaze::ecs {
	template<typename T>
	concept Bundle = requires(T t) {
		{ std::move(t).components() }; // returns tuple of components
	} && !requires(T& bundle) {
		{ bundle.components() };
	};

	template<Bundle B>
	using BundleComponentTypes = std::remove_cvref_t<decltype(std::declval<B>().components())>;

	template<Component... Cs>
	struct ComponentBundle {
		std::tuple<Cs...> m_components;

		constexpr auto components() && noexcept { return std::move(m_components); }
		constexpr auto components() & = delete;
	};

	template<Bundle B>
	constexpr void visit_bundle(B&& bundle, auto&& func) {
		std::apply([&]<Component... Cs>(Cs&& ... components) {
			(func(std::forward<Cs>(components)), ...);
		}, std::forward<B>(bundle).components());
	}

	template<Bundle B>
	constexpr void visit_bundle_types(auto&& func) {
		[]<Component... Cs>(auto&& f, std::type_identity<std::tuple<Cs...>>) {
			(f.template operator()<Cs>(), ...);
		}(func, std::type_identity<BundleComponentTypes<B>>{});
	}

	template<Bundle B>
	[[nodiscard]] consteval size_t bundle_components_count() noexcept {
        return std::tuple_size_v<BundleComponentTypes<B>>;
	}

	template<Bundle B>
	[[nodiscard]] consteval size_t bundle_table_components_count() noexcept {
		size_t count = 0;
		visit_bundle_types<B>([&count]<Component C>() {
			if constexpr (get_storage_type<C>() == StorageType::Table) {
				count++;
			}
		});
		return count;
	}

	template<Bundle B>
	[[nodiscard]] consteval size_t bundle_sparse_components_count() noexcept {
		size_t count = 0;
		visit_bundle_types<B>([&count]<Component C>() {
			if constexpr (get_storage_type<C>() == StorageType::SparseSet) {
				count++;
			}
		});
		return count;
	}

	template<Bundle B>
	[[nodiscard]] consteval auto bundle_components_storage_type() noexcept {
		constexpr auto N = bundle_components_count<B>();
		std::vector<StorageType> storages(N);
		size_t i = 0;
		visit_bundle_types<B>([&storages, &i]<Component C>() {
			storages[i++] = get_storage_type<C>();
		});
		return std::span<const StorageType>(storages);
	}

	struct BundleComponents {
		BundleComponents(const BundleId id, std::vector<ComponentId>&& components, std::vector<StorageType>&& storages) noexcept
			: m_id(id), m_components(std::move(components)), m_storages(std::move(storages)) {
		}

		template<Bundle B>
		[[nodiscard]] static BundleComponents create(const BundleId id, ComponentManager& component_manager) {
			std::vector<ComponentId> component_ids;
			component_ids.reserve(bundle_components_count<B>());

			std::vector<StorageType> storages;
			storages.reserve(bundle_components_count<B>());

			visit_bundle_types<B>([&component_manager, &component_ids, &storages]<Component C>() {
				component_ids.push_back(component_manager.register_component<C>());
				storages.push_back(get_storage_type<C>());
			});

			const auto unique_res = std::ranges::unique(component_ids);
			if (unique_res.begin() != unique_res.end()) {
				utils::panic("Bundle {} has duplicate components {}", id.get(), unique_res | std::views::transform([&component_manager](const auto component_id) {
					return component_manager.get_name(component_id);
				}));
			}

			return BundleComponents{id, std::move(component_ids), std::move(storages)};
		}

		BundleComponents(const BundleComponents& other) = delete;
		BundleComponents& operator=(const BundleComponents& other) = delete;

		BundleComponents(BundleComponents&& other) noexcept = default;
		BundleComponents& operator=(BundleComponents&& other) noexcept = default;

		[[nodiscard]] BundleId id() const noexcept { return m_id; }
		[[nodiscard]] std::span<const ComponentId> components() const noexcept { return m_components; }
		[[nodiscard]] std::span<const StorageType> storages() const noexcept { return m_storages; }

		[[nodiscard]] auto table_components() const noexcept {
			return std::views::zip(m_components, m_storages)
				 | std::views::filter([](const auto& pair) { return std::get<1>(pair) == StorageType::Table; })
				 | std::views::elements<0>;
		}

		[[nodiscard]] auto sparse_components() const noexcept {
			return std::views::zip(m_components, m_storages)
				 | std::views::filter([](const auto& pair) { return std::get<1>(pair) == StorageType::SparseSet; })
				 | std::views::elements<0>;
		}

		[[nodiscard]] size_t table_components_count() const noexcept { return std::ranges::count(m_storages, StorageType::Table); }
		[[nodiscard]] size_t sparse_components_count() const noexcept { return std::ranges::count(m_storages, StorageType::SparseSet); }

		[[nodiscard]] size_t size() const noexcept { return m_components.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_components.empty(); }

	private:
		BundleId m_id;
		std::vector<ComponentId> m_components;
		std::vector<StorageType> m_storages;
	};

	struct BundleManager {
		BundleManager() noexcept = default;

		BundleManager(const BundleManager& other) = delete;
		BundleManager& operator=(const BundleManager& other) = delete;

		BundleManager(BundleManager&& other) = delete;
		BundleManager& operator=(BundleManager&& other) = delete;

		template<Bundle B>
		BundleId register_bundle(ComponentManager& component_manager) {
			constexpr auto type_id = utils::type_id<B>();
			const auto it = m_bundle_map.find(utils::type_id<B>());
			if (it != m_bundle_map.end()) {
				return it->second;
			}

			const auto bundle_id = BundleId::from_index(m_bundles.size());
			m_bundles.push_back(BundleComponents::create<B>(bundle_id, component_manager));
			m_bundle_map.emplace(type_id, bundle_id);
			return bundle_id;
		}

		[[nodiscard]] std::span<const BundleComponents> bundles() const noexcept { return m_bundles; }

		template<Bundle B>
		[[nodiscard]] BundleId bundle_id() const noexcept {
			return get_id(utils::TypeInfo::of<std::remove_cvref_t<B>>());
		}

		[[nodiscard]] BundleId bundle_id(const utils::TypeInfo& type_info) const noexcept {
			const auto it = m_bundle_map.find(type_info);
			if (it == m_bundle_map.end()) {
				return utils::null_id;
			}
			return it->second;
		}

		[[nodiscard]] std::span<const StorageType> storage_types(const BundleId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_bundles.size()) {
				utils::panic("Bundle id {} is out of range", id.get());
			}
			return m_bundles[index].storages();
		}

		[[nodiscard]] auto& operator[](this auto& self, const BundleId id) noexcept {
			return self.m_bundles[id.to_index()];
		}

		[[nodiscard]] auto& at(this auto& self, const BundleId id) noexcept {
			const auto index = id.to_index();
			if (index >= self.m_bundles.size()) {
				utils::panic("Bundle id {} is out of range", id.get());
			}

			return self.m_bundles.at(index);
		}

		[[nodiscard]] size_t size() const noexcept { return m_bundles.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_bundles.empty(); }

	private:
		std::vector<BundleComponents> m_bundles;
		utils::TypeInfoMap<BundleId> m_bundle_map;
	};
}