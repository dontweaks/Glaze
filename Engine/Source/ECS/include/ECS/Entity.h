#pragma once
#include <cstdint>
#include <format>
#include <optional>
#include <vector>

#include "Ids.h"

namespace glaze::ecs {
	struct EntityLocation {
		ArchetypeId archetype_id;
		ArchetypeRow archetype_row;
		TableId table_id;
		TableRow table_row;
	};

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

	struct EntityManager {
		struct Slot {
			EntityIndex next = EntityIndex::INVALID_VALUE;
			EntityVersion version = FIRST_ENTITY_VERSION;
		};

		[[nodiscard]] Entity create_entity() {
			if (m_destroyed == 0) {
				const auto [next, version] = m_slots.emplace_back(EntityIndex::from_index(m_slots.size()));
				return Entity{next, version};
			}

			const EntityIndex index = m_head;
			auto& [next, version] = m_slots[index.to_index()];
			m_head = next;
			m_destroyed--;
			return Entity{index, version};
		}

		bool destroy_entity(const Entity entity) noexcept {
			const auto index = entity.index().get();
			if (index >= m_slots.size()) {
				return false;
			}

			auto& [next, version] = m_slots[index];
			if (version != entity.version()) {
				return false;
			}

			next = m_head;
			m_head = entity.index();
			++version;
			m_destroyed++;

			return true;
		}

		[[nodiscard]] std::optional<Entity> entity(const EntityIndex index) const noexcept {
			const auto i = index.to_index();
			if (i >= m_slots.size()) {
				return std::nullopt;
			}
			const auto& [next, version] = m_slots[i];
			return Entity{index, version};
		}

		[[nodiscard]] bool is_valid(const Entity entity) const noexcept {
			const auto index = entity.index().get();
			if (index >= m_slots.size()) {
				return false;
			}

			const auto [next, version] = m_slots[index];
			if (version != entity.version()) {
				return false;
			}

			return true;
		}

		[[nodiscard]] size_t size() const noexcept {
			return m_slots.size() - m_destroyed;
		}

		[[nodiscard]] size_t max_size() const noexcept {
			return m_slots.size();
		}

		void clear() noexcept {
			m_slots.clear();
			m_destroyed = 0;
			m_head = EntityIndex::INVALID_VALUE;
		}

	private:
		std::vector<Slot> m_slots;
		size_t m_destroyed = 0;
		EntityIndex m_head = EntityIndex::INVALID_VALUE;
	};
}

template<>
struct std::hash<glaze::ecs::Entity> {
	size_t operator()(const glaze::ecs::Entity& entity) const noexcept {
		return std::hash<uint64_t>()(entity.to_id().get());
	}
};

template<>
struct std::formatter<glaze::ecs::Entity> {
	static constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	static auto format(const glaze::ecs::Entity& entity, std::format_context& ctx) {
		return std::format_to(ctx.out(), "Entity(index: {}, version: {})", entity.index().get(), entity.version().get());
	}
};