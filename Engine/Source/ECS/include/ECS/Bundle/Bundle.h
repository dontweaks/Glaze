#pragma once

#include <type_traits>
#include <tuple>

namespace glaze::ecs {
	template<typename T>
	concept Bundle = std::is_class_v<std::remove_cvref_t<T>>;

	template<Bundle ... Ts>
	struct TupleBundle {
		std::tuple<Ts...> components;
	};
}