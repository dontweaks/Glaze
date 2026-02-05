#pragma once

namespace glaze::utils {
	struct TypeOps {
		template<typename T>
		[[nodiscard]] static consteval TypeOps of() {
			return TypeOps{};
		}
	};
}