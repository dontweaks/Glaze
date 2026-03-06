#pragma once

#include "ECS/Storage/SparseSet/SparseSet.h"
#include "ECS/Component/ComponentManager.h"

#include "Bundle.h"

namespace glaze::ecs {
	struct BundleMeta {
		template<Bundle B>
		[[nodiscard]] static BundleMeta create(const BundleId id, ComponentManager& component_manager) {
			SparseSet<ComponentId, BundleComponent> components;
			components.reserve(bundle_components_count<B>());

			visit_bundle_types<B>([&component_manager, &components, id]<Component C>() {
				const auto component_id = component_manager.register_component<C>();
				const auto name = component_manager.get_name(component_id).value_or("Error");
				if (components.contains(component_id)) {
					utils::panic("Bundle {} has duplicate components {}", id.get(), name);
				}
				components.emplace(component_id, get_storage_type<C>(), name);
			});

			return BundleMeta{id, std::move(components)};
		}

		BundleMeta(const BundleMeta& other) = delete;
		BundleMeta& operator=(const BundleMeta& other) = delete;

		BundleMeta(BundleMeta&& other) noexcept = default;
		BundleMeta& operator=(BundleMeta&& other) noexcept = default;

		[[nodiscard]] BundleId id() const noexcept { return m_id; }
		[[nodiscard]] std::span<const ComponentId> components() const noexcept { return m_components.indices(); }

		[[nodiscard]] auto storages() const noexcept {
			return m_components.values() | std::views::transform([](const auto bundle_component) {
				return bundle_component.storage;
			});
		}

		[[nodiscard]] auto names() const noexcept {
			return m_components.values() | std::views::transform([](const auto bundle_component) {
				return bundle_component.name;
			});
		}

		[[nodiscard]] auto table_components() const noexcept {
			return m_components.iter()
				| std::views::filter([](const auto& p) { return std::get<1>(p).storage == StorageType::Table; })
				| std::views::elements<0>;
		}

		[[nodiscard]] auto sparse_components() const noexcept {
			return m_components.iter()
				| std::views::filter([](const auto& p) { return std::get<1>(p).storage == StorageType::SparseSet; })
				| std::views::elements<0>;
		}

		[[nodiscard]] size_t table_components_count() const noexcept { return std::ranges::count(m_components.values(), StorageType::Table); }
		[[nodiscard]] size_t sparse_components_count() const noexcept { return std::ranges::count(m_components.values(), StorageType::SparseSet); }

		[[nodiscard]] size_t size() const noexcept { return m_components.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_components.empty(); }

	private:
		struct BundleComponent {
			operator StorageType() const noexcept { return storage; }

			StorageType storage;
			std::string_view name;
		};

		BundleMeta(const BundleId id, SparseSet<ComponentId, BundleComponent>&& components) noexcept
			: m_id(id), m_components(std::move(components)) {
		}

		BundleId m_id;
		SparseSet<ComponentId, BundleComponent> m_components;
	};
}

template<>
struct std::formatter<glaze::ecs::BundleMeta> {
	static constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	static auto format(const glaze::ecs::BundleMeta& bundle, std::format_context& ctx) {
		return std::format_to(ctx.out(), "Bundle: {}", bundle.names());
	}
};