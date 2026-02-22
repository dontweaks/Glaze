#pragma once

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

	template<typename Tuple>
	struct NormalizeBundleTuple;

	template<Component... Cs>
	struct NormalizeBundleTuple<std::tuple<Cs...>> {
		using type = std::tuple<std::remove_cvref_t<Cs>...>;
	};

	template<typename Tuple>
	using NormalizeBundleTupleT = NormalizeBundleTuple<std::remove_cvref_t<Tuple>>::type;

	template<Bundle B>
	using BundleKeyType = NormalizeBundleTupleT<BundleComponentTypes<B>>;

	template<Component... Cs>
	struct ComponentBundle {
		static_assert((std::is_reference_v<Cs> && ...));

		std::tuple<Cs...> m_components;

		constexpr explicit ComponentBundle(Cs... cs) noexcept
			: m_components(std::forward<Cs>(cs)...) {}

		constexpr auto components() && noexcept { return std::move(m_components); }
		constexpr auto components() & = delete;
	};

	template<Component... Cs>
	ComponentBundle(Cs&&...) -> ComponentBundle<Cs&&...>;

	template<Bundle B> requires (!std::is_lvalue_reference_v<B>)
	constexpr void visit_bundle(B&& bundle, auto&& func) {
		std::apply([&]<Component... Cs>(Cs&&... components) {
				(func(std::forward_like<B>(components)), ...);
			}, std::forward<B>(bundle).components()
		);
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
}