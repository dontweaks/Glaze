#pragma once
#include <cstdint>

#include "Ids.h"

namespace glaze::ecs {
	struct Entity {
		constexpr explicit Entity(const EntityIndex index, const EntityVersion version = FIRST_ENTITY_VERSION) noexcept
			: m_index(index), m_version(version) {
		}

		[[nodiscard]] constexpr static Entity from_id(const EntityID id) noexcept {
			const auto index = EntityIndex(id.get());
			const auto version = EntityVersion(static_cast<uint32_t>(id.get() >> 32));
			return Entity{ index, version };
		}

		[[nodiscard]] constexpr EntityID to_id() const noexcept {
			return EntityID { static_cast<uint64_t>(m_index.get()) | static_cast<uint64_t>(m_version.get()) << 32 };
		}

		[[nodiscard]] constexpr EntityIndex index() const noexcept { return m_index; }
		[[nodiscard]] constexpr EntityVersion version() const noexcept { return m_version; }

		[[nodiscard]] constexpr auto operator<=>(const Entity& other) const noexcept { return to_id() <=> other.to_id(); }

	private:
		EntityIndex m_index;
		EntityVersion m_version;
	};
}
