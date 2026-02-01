#pragma once

#include "Utils/StrongId.h"

namespace glaze::ecs {
	using EntityIndex = utils::StrongId<struct EntityIndexTag, uint32_t>;
	using EntityVersion = utils::StrongId<struct EntityVersionTag, uint32_t>;
	using EntityID = utils::StrongId<struct EntityIDTag, uint64_t>;

	static constexpr EntityVersion FIRST_ENTITY_VERSION{0};

	using WorldId = utils::StrongId<struct WorldIdTag, uint64_t>;
}