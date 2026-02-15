#pragma once

#include <string_view>
#include <format>
#include <unordered_map>

namespace glaze::utils {
	namespace details {
		[[nodiscard]] consteval uint64_t fnv1a_64(const std::string_view s) noexcept {
			uint64_t h = 14695981039346656037ull;
			for (const uint8_t c : s) {
				h ^= static_cast<uint64_t>(c);
				h *= 1099511628211ull;
			}
			return h;
		}

		template<typename T>
	   [[nodiscard]] consteval auto stripped_type_name() noexcept {
#if defined(__clang__) || defined(__GNUC__)
			constexpr auto pretty_function = __PRETTY_FUNCTION__;
			constexpr auto pretty_function_prefix = '=';
			constexpr auto pretty_function_suffix = ']';
#elif defined(_MSC_VER)
			constexpr auto pretty_function = __FUNCSIG__;
			constexpr auto pretty_function_prefix = '<';
			constexpr auto pretty_function_suffix = '>';
#else
			#error "Unsupported compiler"
#endif
			constexpr std::string_view full_name = pretty_function;

			constexpr auto first = full_name.find_first_not_of(' ', full_name.find_first_of(pretty_function_prefix) + 1);
			constexpr auto last = full_name.find_last_of(pretty_function_suffix);
			return full_name.substr(first, last - first);
		}

		template<auto Id>
		struct Counter {
			using Tag = Counter;

			struct Generator {
				[[nodiscard]] friend consteval auto is_defined(Tag) noexcept {
					return true;
				}
			};
			friend consteval auto is_defined(Tag) noexcept;

			template<typename T = Tag, auto = is_defined(T{})>
			[[nodiscard]] static consteval auto exist(auto) noexcept {
				return true;
			}

			[[nodiscard]] static consteval auto exist(...) noexcept {
				return Generator(), false;
			}
		};

		template<typename T, auto Id = uint64_t{}>
		[[nodiscard]] consteval auto type_id() noexcept {
			if constexpr (Counter<Id>::exist(Id)) {
				return type_id<T, Id + 1>();
			} else {
				return Id;
			}
		}

		template<typename T>
		[[nodiscard]] consteval std::string_view type_name() noexcept {
			return stripped_type_name<T>();
		}

		template<typename T>
		[[nodiscard]] consteval uint64_t type_hash() noexcept {
			return fnv1a_64(stripped_type_name<T>());
		}
	}

	template<typename T>
	struct TypeIndex {
		static constexpr uint64_t value = details::type_id<T>();
		constexpr operator uint64_t() const noexcept { return value; }
	};

	template<typename T>
	struct TypeHash {
		static constexpr uint64_t value = details::fnv1a_64(details::stripped_type_name<T>());
		constexpr operator uint64_t() const noexcept { return value; }
	};

	template<typename T>
	struct TypeName {
		static constexpr std::string_view value = details::stripped_type_name<T>();
		constexpr operator std::string_view() const noexcept { return value; }
	};

	struct TypeInfo {
		template<typename T>
		explicit constexpr TypeInfo(std::in_place_type_t<T>) noexcept
			: m_hash(TypeHash<T>::value),
		      m_name(TypeName<T>::value) {
		}

		constexpr TypeInfo() noexcept = default;

		template<typename T>
		[[nodiscard]] static consteval TypeInfo of() noexcept {
			return TypeInfo(std::in_place_type<T>);
		}

		[[nodiscard]] constexpr uint64_t hash() const noexcept {
			return m_hash;
		}

		[[nodiscard]] constexpr std::string_view name() const noexcept {
			return m_name;
		}

		[[nodiscard]] constexpr auto operator<=>(const TypeInfo&) const noexcept = default;

	private:
		uint64_t m_hash{};
		std::string_view m_name;
	};

	template<typename T>
	[[nodiscard]] constexpr TypeInfo type_id() noexcept {
		static const TypeInfo instance{std::in_place_type<std::remove_cvref_t<T>>};
		return instance;
	}

	template<typename T>
	[[nodiscard]] const TypeInfo& type_id(T&&) noexcept {
		return type_id<std::remove_cvref_t<T>>();
	}

	template<typename T>
	using TypeInfoMap = std::unordered_map<TypeInfo, T>;
}

template<>
struct std::hash<glaze::utils::TypeInfo> {
	constexpr size_t operator()(const glaze::utils::TypeInfo& info) const noexcept {
		return info.hash();
	}
};

template <>
struct std::formatter<glaze::utils::TypeInfo> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	template<typename FormatContext>
	constexpr auto format(const glaze::utils::TypeInfo& info, FormatContext& ctx) const {
		return std::format_to(
			ctx.out(),
			"TypeInfo{{name={}, hash={}}}",
			info.name(),
			info.hash()
		);
	}
};
