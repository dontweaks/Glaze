#pragma once

#include <concepts>
#include <limits>

namespace glaze::utils {
	template<typename Tag, std::unsigned_integral I>
	struct [[nodiscard]] StrongId {
		static constexpr auto MIN = std::numeric_limits<I>::min();
		static constexpr auto MAX = std::numeric_limits<I>::max();

		static const StrongId INVALID_VALUE;

		static constexpr StrongId from_index(const size_t index) noexcept {
			if (index >= static_cast<std::size_t>(MAX)) [[unlikely]] {
				return INVALID_VALUE;
			}
			return StrongId{static_cast<I>(index)};
		}
		constexpr size_t to_index() const noexcept { return static_cast<size_t>(m_value); }

		constexpr StrongId() noexcept = default;
		constexpr explicit StrongId(const I value) noexcept : m_value(value) {}

		constexpr I get() const noexcept { return m_value; }
		explicit constexpr operator I() const noexcept { return m_value; }
		constexpr auto operator<=>(const StrongId&) const noexcept = default;

		constexpr bool valid() const noexcept { return m_value != INVALID_VALUE; }

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