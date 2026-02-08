#pragma once

#include <tuple>

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

		constexpr auto components() && { return std::move(m_components); }
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
}