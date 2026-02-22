#pragma once

#include "ECS/Ids.h"

namespace glaze::ecs {
	enum struct StorageType : uint8_t {
		Table,
		SparseSet
	};

	template<typename T>
	concept Component = std::is_standard_layout_v<std::remove_cvref_t<T>>;

	template<typename T>
	concept HasStorageType = Component<T> && requires {
		{ std::remove_cvref_t<T>::STORAGE_TYPE } -> std::convertible_to<StorageType>;
	};

	template<Component T>
	[[nodiscard]] consteval StorageType get_storage_type() noexcept {
		using U = std::remove_cvref_t<T>;
		if constexpr (HasStorageType<U>) {
			return U::STORAGE_TYPE;
		} else {
			return StorageType::Table;
		}
	}
}
