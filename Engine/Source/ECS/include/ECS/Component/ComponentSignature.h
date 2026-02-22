#pragma once

#include <vector>
#include <unordered_map>

#include "ECS/Ids.h"
#include "Utils/HashCombine.h"

namespace glaze::ecs {
	struct ComponentSignatureView {
		std::span<const ComponentId> table;
		std::span<const ComponentId> sparse;
	};

	struct ComponentSignature {
		ComponentSignature() = default;
		ComponentSignature(
			const std::span<const ComponentId> table,
			const std::span<const ComponentId> sparse) noexcept
			: table(table.begin(), table.end()), sparse(sparse.begin(), sparse.end()) {
		}

		explicit ComponentSignature(const ComponentSignatureView view) noexcept
			: ComponentSignature(view.table, view.sparse) {
		}

		[[nodiscard]] size_t component_count() const noexcept {
			return table.size() + sparse.size();
		}

		std::vector<ComponentId> table;
		std::vector<ComponentId> sparse;
	};

	struct ComponentSignatureHasher {
		using is_transparent = void;

		[[nodiscard]] size_t operator()(const ComponentSignature& k) const noexcept {
			return hash_view({k.table, k.sparse});
		}

		[[nodiscard]] size_t operator()(const ComponentSignatureView v) const noexcept {
			return hash_view(v);
		}

	private:
		[[nodiscard]] static size_t hash_view(const ComponentSignatureView v) noexcept {
			size_t h = 0;
			utils::hash_combine_with<ComponentIdHasher>(h, v.table, v.sparse);
			return h;
		}
	};

	struct ComponentSignatureEq {
		using is_transparent = void;

		[[nodiscard]] bool operator()(const ComponentSignature& a, const ComponentSignature& b) const noexcept {
			return equal({a.table, a.sparse}, {b.table, b.sparse});
		}

		[[nodiscard]] bool operator()(const ComponentSignature& a, const ComponentSignatureView b) const noexcept {
			return equal({a.table, a.sparse}, b);
		}

		[[nodiscard]] bool operator()(const ComponentSignatureView a, const ComponentSignature& b) const noexcept {
			return equal(a, {b.table, b.sparse});
		}

	private:
		[[nodiscard]] static bool equal(const ComponentSignatureView a, const ComponentSignatureView b) noexcept {
			return a.table.size() == b.table.size()
				&& a.sparse.size() == b.sparse.size()
				&& std::ranges::equal(a.table, b.table)
				&& std::ranges::equal(a.sparse, b.sparse);
		}
	};

	template<typename T>
	using ByComponentsMap = std::unordered_map<
		ComponentSignature,
		T,
		ComponentSignatureHasher,
		ComponentSignatureEq
	>;
}