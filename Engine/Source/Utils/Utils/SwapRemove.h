#pragma once

#include <vector>

#include "Panic.h"

namespace glaze::utils {
	template<typename T>
	constexpr T swap_remove(std::vector<T>& vec, size_t index) noexcept {
		static_assert(std::is_nothrow_move_constructible_v<T>);
		static_assert(std::is_nothrow_move_assignable_v<T>);
		if (vec.size() <= index) {
			panic("swap_remove index is ouf of range");
		}

		T val = std::exchange(vec[index], std::move(vec.back()));
		vec.pop_back();
		return val;
	}
}