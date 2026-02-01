#pragma once
#include <cstdint>

namespace glaze::ecs {
	struct Entity {
		using Index = uint32_t;
		using Version = uint32_t;
		using ID = uint64_t;

		static constexpr Version FIRST_VERSION = 0;

		constexpr explicit Entity(const Index index, const Version version = FIRST_VERSION) noexcept
			: m_index(index), m_version(version) {
		}

		[[nodiscard]] constexpr static Entity from_id(const ID id) noexcept {
			return Entity {
				static_cast<Index>(id),
				static_cast<Version>(id >> 32)
			};
		}

		[[nodiscard]] constexpr ID to_id() const noexcept {
			return static_cast<ID>(m_index) | static_cast<ID>(m_version) << 32;
		}

		[[nodiscard]] constexpr Index index() const noexcept { return m_index; }
		[[nodiscard]] constexpr Version version() const noexcept { return m_version; }

		[[nodiscard]] constexpr auto operator<=>(const Entity& other) const noexcept { return to_id() <=> other.to_id(); }

	private:
		Index m_index;
		Version m_version;
	};
}
