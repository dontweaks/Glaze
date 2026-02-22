#pragma once

#include "ComponentMeta.h"

namespace glaze::ecs {
	struct ComponentManager {
		ComponentManager() = default;

		ComponentManager(const ComponentManager& other) = delete;
		ComponentManager& operator=(const ComponentManager& other) = delete;

		ComponentManager(ComponentManager&& other) = delete;
		ComponentManager& operator=(ComponentManager&& other) = delete;

		template<Component T>
		ComponentId register_component() {
			constexpr auto type_id = utils::TypeInfo::of<std::remove_cvref_t<T>>();
			const auto indices_it = m_components_map.find(type_id);
			if (indices_it != m_components_map.end()) {
				return indices_it->second;
			}

			const auto id = ComponentId::from_index(m_components.size());
			m_components.emplace_back(id, ComponentDesc::of<T>());
			m_components_map.emplace(type_id, id);

			return id;
		}

		template<Component T>
		[[nodiscard]] ComponentId component_id() const noexcept {
			return component_id(utils::TypeInfo::of<std::remove_cvref_t<T>>());
		}

		[[nodiscard]] ComponentId component_id(const utils::TypeInfo& type_info) const noexcept {
			const auto it = m_components_map.find(type_info);
			if (it == m_components_map.end()) {
				return utils::null_id;
			}
			return it->second;
		}

		[[nodiscard]] const ComponentMeta* get_meta(const ComponentId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_components.size()) {
				return nullptr;
			}

			return &m_components[index];
		}

		[[nodiscard]] const ComponentDesc* get_desc(const ComponentId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_components.size()) {
				return nullptr;
			}
			return &m_components[index].m_desc;
		}

		[[nodiscard]] std::optional<std::string_view> get_name(const ComponentId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_components.size()) {
				return std::nullopt;
			}
			return m_components[index].m_desc.name();
		}

		[[nodiscard]] auto& operator[](this auto& self, const ComponentId id) noexcept { return self.m_components[id.to_index()]; }

		[[nodiscard]] bool is_id_valid(const ComponentId id) const noexcept {
			return id.to_index() < m_components.size() && id.valid();
		}

		[[nodiscard]] size_t size() const noexcept { return m_components.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_components.empty(); }

	private:
		std::vector<ComponentMeta> m_components;
		utils::TypeInfoMap<ComponentId> m_components_map;
	};
}