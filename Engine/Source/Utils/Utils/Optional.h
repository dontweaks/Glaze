#pragma once

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#include "Panic.h"

namespace glaze::utils {
	template<typename T>
	inline constexpr bool is_optional_v = false;

	template<typename T>
	inline constexpr bool is_optional_v<std::optional<T>> = true;

	template<typename T>
	using optional_ref = std::optional<std::reference_wrapper<T>>;

	template<typename O>
	concept optional_type = is_optional_v<std::remove_cvref_t<O>>;

	template<optional_type O>
	[[nodiscard]] decltype(auto) value_or_panic(O&& opt) {
		if (!opt) {
			panic("Optional is empty");
		}
		using T = std::remove_cvref_t<O>::value_type;
		if constexpr (!std::is_same_v<T, std::unwrap_reference_t<T>>) {
			return opt->get();
		} else if constexpr (!std::is_lvalue_reference_v<O>) {
			return std::remove_cvref_t<T>(std::move(*opt));
		} else {
			return *opt;
		}
	}

	template<optional_type O>
	[[nodiscard]] decltype(auto) value_or_panic_debug(O&& opt) {
#ifndef NDEBUG
		if (!opt) {
			panic("Optional is empty");
		}
#endif
		using T = std::remove_cvref_t<O>::value_type;
		if constexpr (!std::is_same_v<T, std::unwrap_reference_t<T>>) {
			return opt->get();
		} else if constexpr (!std::is_lvalue_reference_v<O>) {
			return std::remove_cvref_t<T>(std::move(*opt));
		} else {
			return *opt;
		}
	}
}