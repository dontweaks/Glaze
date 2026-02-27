#pragma once

#include <type_traits>
#include <utility>
#include <memory>

namespace glaze::utils {
	struct TypeOps {
		using ConstructorFn = void(*)(void* obj);
		using DestructorFn  = void(*)(void* obj) noexcept;

		using MoveCtorFn    = void(*)(void* dst, void* src) noexcept;
		using MoveAssignFn  = void(*)(void* dst, void* src) noexcept;

		using CopyCtorFn    = void(*)(void* dst, const void* src);
		using CopyAssignFn  = void(*)(void* dst, const void* src);

		template<typename T>
		[[nodiscard]] static consteval TypeOps of() {
			using U = std::remove_cvref_t<T>;

			static_assert(std::is_nothrow_destructible_v<U>,
				"TypeOps requires nothrow destructible types (destruct is noexcept).");
			static_assert(std::is_nothrow_move_constructible_v<U>,
				"TypeOps requires nothrow move constructible types (move constructor is noexcept).");
			static_assert(std::is_nothrow_move_assignable_v<U>,
				"TypeOps requires nothrow move assignable types (move operator is noexcept).");

			TypeOps ops{};

			if constexpr (std::is_default_constructible_v<U>) {
				ops.construct = [](void* const obj) noexcept(std::is_nothrow_default_constructible_v<U>) {
					std::construct_at(static_cast<U*>(obj));
				};
			}

			ops.destruct = [](void* const obj) noexcept {
				std::destroy_at(static_cast<U*>(obj));
			};

			ops.move_construct = [](void* const dst, void* const src) noexcept {
				std::construct_at(static_cast<U*>(dst), std::move(*static_cast<U*>(src)));
			};

			ops.move_assign = [](void* const dst, void* const src) noexcept {
				if (dst != src) {
					*static_cast<U*>(dst) = std::move(*static_cast<U*>(src));
				}
			};

			if constexpr (std::is_copy_constructible_v<U>) {
				ops.copy_construct = [](void* const dst, const void* const src) noexcept(std::is_nothrow_copy_constructible_v<U>) {
					std::construct_at(static_cast<U*>(dst), *static_cast<const U*>(src));
				};
			}

			if constexpr (std::is_copy_assignable_v<U>) {
				ops.copy_assign = [](void* const dst, const void* const src) noexcept(std::is_nothrow_copy_assignable_v<U>) {
					if (dst != src) {
						*static_cast<U*>(dst) = *static_cast<const U*>(src);
					}
				};
			}

			return ops;
		}

		ConstructorFn construct = nullptr;
		DestructorFn  destruct  = nullptr;

		MoveCtorFn    move_construct = nullptr;
		MoveAssignFn  move_assign    = nullptr;

		CopyCtorFn    copy_construct = nullptr;
		CopyAssignFn  copy_assign    = nullptr;
	};
}