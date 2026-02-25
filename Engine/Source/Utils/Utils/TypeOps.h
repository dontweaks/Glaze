#pragma once

namespace glaze::utils {
	struct TypeOps {
		using ConstructorFn = void(*)(void* const obj);
		using DestructorFn = void(*)(void* const obj) noexcept;

		template<typename T>
		[[nodiscard]] static consteval TypeOps of() {
			using U = std::remove_cvref_t<T>;

			static_assert(std::is_nothrow_destructible_v<U>, "Object must be nothrow destructible");

			return TypeOps {
				.construct = [](void* const obj) {
					std::construct_at(static_cast<U* const>(obj));
				},
				.destruct = [](void* const obj) noexcept {
					std::destroy_at(static_cast<U* const>(obj));
				}
			};
		}

		ConstructorFn construct;
		DestructorFn destruct;
	};
}