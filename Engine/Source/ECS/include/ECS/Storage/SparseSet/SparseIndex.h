#pragma once

#include <concepts>

namespace glaze::ecs {
	template<typename I>
	concept SparseIndex = requires (I i) {
		{ i.to_index() } noexcept -> std::same_as<size_t>;
	};
}