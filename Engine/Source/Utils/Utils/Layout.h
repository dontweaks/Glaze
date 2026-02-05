#pragma once

#include <compare>
#include <format>

namespace glaze::utils {
	struct [[nodiscard]] Layout {
		template<typename T>
		static consteval Layout of() noexcept { return Layout { sizeof(T), alignof(T) }; }

		constexpr size_t size() const noexcept { return m_size; }
		constexpr size_t align() const noexcept { return m_align; }

		constexpr auto operator<=>(const Layout&) const noexcept = default;

	private:
		consteval Layout(const size_t size, const size_t alignment) noexcept
			: m_size(size), m_align(alignment) {
		}

		size_t m_size = 0;
		size_t m_align = 1;
	};
}

template <>
struct std::formatter<glaze::utils::Layout> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	template<typename FormatContext>
	constexpr auto format(const glaze::utils::Layout& l, FormatContext& ctx) const {
		return std::format_to(
			ctx.out(),
			"Layout{{size={}, align={}}}",
			l.size(),
			l.align()
		);
	}
};
