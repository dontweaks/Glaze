#pragma once

#include <compare>
#include <format>

namespace glaze::utils {
	struct Layout {
		template<typename T>
		[[nodiscard]] static consteval Layout of() noexcept {
			using U = std::remove_cvref_t<T>;
			if constexpr (std::is_empty_v<U>) {
				return Layout {
					0,
					alignof(U)
				};
			} else {
				return Layout {
					sizeof(U),
					alignof(U)
				};
			}
		}

		constexpr Layout() noexcept = default;
		constexpr Layout(const size_t size, const size_t alignment) noexcept
			: m_size(size), m_align(alignment) {
		}

		[[nodiscard]] constexpr size_t size() const noexcept { return m_size; }
		[[nodiscard]] constexpr size_t align() const noexcept { return m_align; }

		[[nodiscard]] constexpr auto operator<=>(const Layout&) const noexcept = default;

	private:
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
