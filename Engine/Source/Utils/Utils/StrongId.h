#pragma once

#include <concepts>
#include <limits>
#include <compare>

namespace glaze::utils {
	template<typename Tag, std::unsigned_integral I>
	struct StrongId {
		using Type = I;
		static constexpr auto MIN = std::numeric_limits<I>::min();
		static constexpr auto MAX = std::numeric_limits<I>::max();

		static const StrongId INVALID_VALUE;

		[[nodiscard]] static constexpr StrongId from_index(const size_t index) noexcept {
			if (index >= static_cast<std::size_t>(MAX)) [[unlikely]] {
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

		[[nodiscard]] constexpr bool valid() const noexcept { return m_value != MAX; }

		StrongId& operator++() noexcept {
			++m_value;
			return *this;
		}

		StrongId operator++(int) noexcept {
			StrongId id(*this);
			++*this;
			return id;
		}

	protected:
		I m_value = MAX;
	};

	template<typename Tag, std::unsigned_integral I>
	constexpr StrongId<Tag, I> StrongId<Tag, I>::INVALID_VALUE = StrongId(MAX);
}

template<typename Tag, std::integral I>
struct std::hash<glaze::utils::StrongId<Tag, I>> {
	[[nodiscard]] size_t operator()(const glaze::utils::StrongId<Tag, I> v) const noexcept {
		return std::hash<I>{}(v.get());
	}
};