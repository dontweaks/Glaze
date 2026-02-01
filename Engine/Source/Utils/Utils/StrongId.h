#pragma once

#include <concepts>
#include <limits>
#include <compare>

namespace glaze::utils {
	template<typename Tag, std::integral I>
	struct StrongId {
		static constexpr auto MIN = std::numeric_limits<I>::min();
		static constexpr auto MAX = std::numeric_limits<I>::max();

		static constexpr I INVALID_VALUE = MAX;

		[[nodiscard]] static constexpr StrongId from_index(const size_t index) noexcept {
			if (index >= static_cast<std::size_t>(INVALID_VALUE)) [[unlikely]] {
				return INVALID_VALUE;
			}
			return StrongId{static_cast<I>(index)};
		}
		[[nodiscard]] constexpr size_t to_index() const noexcept { return static_cast<size_t>(m_value); }

		constexpr StrongId() noexcept = default;
		constexpr explicit StrongId(const I value) noexcept : m_value(value) {}

		[[nodiscard]] constexpr I get() const noexcept { return m_value; }
		[[nodiscard]] explicit constexpr operator I() const noexcept { return m_value; }
		[[nodiscard]] constexpr auto operator<=>(const StrongId&) const noexcept = default;

		[[nodiscard]] constexpr bool valid() const noexcept { return m_value != INVALID_VALUE; }

	protected:
		I m_value = INVALID_VALUE;
	};
}