#pragma once

#include <cstdlib>
#include <cstdio>
#include <format>
#include <source_location>

namespace glaze::utils {
	[[noreturn]] inline void panic_impl(const char* msg) noexcept {
		std::fputs(msg, stderr);
		std::abort();
	}

	template <typename... Args>
	struct PanicFormat final {
		template <typename T>
		consteval PanicFormat(const T &s, const std::source_location loc = std::source_location::current()) noexcept
			: fmt{s}, loc{loc} {
		}

		std::format_string<Args...> fmt;
		std::source_location loc;
	};

	template <typename... Args>
	[[noreturn]] void panic(PanicFormat<std::type_identity_t<Args>...> fmt, Args &&...args) noexcept {
		auto user_msg = std::format(fmt.fmt, std::forward<Args>(args)...);
		auto msg = std::format("{}:{} panic: {}\n", fmt.loc.file_name(), fmt.loc.line(), user_msg);
		panic_impl(msg.c_str());
	}
}