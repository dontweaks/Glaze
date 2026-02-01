#pragma once
#include <type_traits>

namespace glaze::ecs {
	template<typename T>
	concept Component = std::is_standard_layout_v<std::remove_cvref_t<T>>;
}
