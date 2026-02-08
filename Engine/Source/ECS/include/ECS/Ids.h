#pragma once

#include <algorithm>
#include <span>

#include "Utils/StrongId.h"

namespace glaze::ecs {
	using EntityIndex = utils::StrongId<struct EntityIndexTag, uint32_t>;
	using EntityVersion = utils::StrongId<struct EntityVersionTag, uint32_t>;
	using EntityID = utils::StrongId<struct EntityIDTag, uint64_t>;
	static constexpr EntityVersion FIRST_ENTITY_VERSION{0};

	using WorldId = utils::StrongId<struct WorldIdTag, uint64_t>;
	using ComponentId = utils::StrongId<struct ComponentIdTag, uint64_t>;

	using ArchetypeId = utils::StrongId<struct ArchetypeIdTag, uint32_t>;
	using ArchetypeVersion = utils::StrongId<struct ArchetypeVersionTag, uint32_t>;
	using ArchetypeRow = utils::StrongId<struct ArchetypeRowTag, uint32_t>;
	static constexpr ArchetypeId EMPTY_ARCHETYPE_ID{0};
	static constexpr ArchetypeVersion FIRST_ARCHETYPE_VERSION{0};

	using TableId = utils::StrongId<struct TableIdTag, uint32_t>;
	using TableRow = utils::StrongId<struct TableRowIdTag, uint32_t>;
	static constexpr TableId EMPTY_TABLE_ID{0};

	struct ComponentIdHasher {
		using is_transparent = void;

		[[nodiscard]] size_t operator()(const std::vector<ComponentId>& key) const noexcept {
			return operator()(std::span(key));
		}

		[[nodiscard]] size_t operator()(const std::span<const ComponentId> key) const noexcept {
			size_t hash = 0;
			for (const auto& id : key) {
				hash ^= std::hash<ComponentId>{}(id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			}
			return hash;
		}
	};
}