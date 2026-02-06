#pragma once

#include <vector>
#include <optional>
#include <unordered_map>

#include "ECS/Ids.h"

#include "Utils/Layout.h"
#include "Utils/TypeOps.h"
#include "Utils/TypeInfo.h"

namespace glaze::ecs {
	enum struct StorageType {
		Table,
		SparseSet
	};

	template<typename T>
	concept HasStorageType = requires {
		{ std::remove_cvref_t<T>::STORAGE_TYPE } -> std::convertible_to<StorageType>;
	};

	template<typename T>
	[[nodiscard]] consteval StorageType get_storage_type() noexcept {
		if constexpr (HasStorageType<T>) {
			return T::STORAGE_TYPE;
		} else {
			return StorageType::Table;
		}
	}

	template<typename T>
	concept Component = std::is_standard_layout_v<std::remove_cvref_t<T>>;

	struct ComponentDesc {
		template<Component T>
		[[nodiscard]] static consteval ComponentDesc of() noexcept {
			using U = std::remove_cvref_t<T>;
			return ComponentDesc {
				utils::TypeName<U>(),
				get_storage_type<U>(),
				utils::Layout::of<U>(),
				utils::TypeOps::of<U>(),
				utils::TypeInfo::of<U>()
			};
		}

		constexpr ComponentDesc(
			const std::string_view name,
			const StorageType storage_type,
			const utils::Layout layout,
			const utils::TypeOps type_ops,
			const utils::TypeInfo& type_info) noexcept
			: m_name(name), m_storage_type(storage_type), m_layout(layout), m_type_ops(type_ops), m_type_info(type_info) {
		}

		[[nodiscard]] constexpr StorageType storage_type() const noexcept { return m_storage_type; }
		[[nodiscard]] constexpr const utils::TypeInfo& type_info() const noexcept { return m_type_info; }
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }

	private:
		friend struct ComponentInfo;

		std::string_view m_name;
		StorageType m_storage_type = StorageType::Table;
		utils::Layout m_layout;
		utils::TypeOps m_type_ops;
		utils::TypeInfo m_type_info{};
	};

	struct ComponentInfo {
		ComponentInfo(const ComponentId id, const ComponentDesc& desc) noexcept
			: m_id(id), m_desc(desc) {
		}

		[[nodiscard]] ComponentId id() const noexcept { return m_id; }
		[[nodiscard]] std::string_view name() const noexcept { return m_desc.m_name; }
		[[nodiscard]] StorageType storage_type() const noexcept { return m_desc.m_storage_type; }
		[[nodiscard]] const utils::Layout& layout() const noexcept { return m_desc.m_layout; }
		[[nodiscard]] const utils::TypeInfo& type_info() const noexcept { return m_desc.m_type_info; }
		[[nodiscard]] const utils::TypeOps& type_ops() const noexcept { return m_desc.m_type_ops; }

	private:
		friend struct ComponentManager;

		ComponentId m_id;
		ComponentDesc m_desc;
	};

	using ComponentInfos = std::vector<ComponentInfo>;
	using ComponentIdTypeInfoMap = std::unordered_map<utils::TypeInfo, ComponentId>;

	struct ComponentManager {
		template<Component T>
		ComponentId register_component() {
			constexpr auto type_id = utils::TypeInfo::of<std::remove_cvref_t<T>>();
			const auto indices_it = m_type_info_map.find(type_id);
			if (indices_it != m_type_info_map.end()) {
				return indices_it->second;
			}

			const auto id = ComponentId::from_index(m_components.size());
			m_components.emplace_back(id, ComponentDesc::of<T>());
			m_type_info_map.emplace(type_id, id);

			return id;
		}

		template<Component T>
		[[nodiscard]] ComponentId component_id() const noexcept {
			return get_id(utils::TypeInfo::of<std::remove_cvref_t<T>>());
		}

		[[nodiscard]] ComponentId get_id(const utils::TypeInfo& type_info) const noexcept {
			const auto it = m_type_info_map.find(type_info);
			if (it == m_type_info_map.end()) {
				return ComponentId::INVALID_VALUE;
			}
			return it->second;
		}

		[[nodiscard]] const ComponentInfo* get_info(const ComponentId id) const noexcept {
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

		[[nodiscard]] bool is_id_valid(const ComponentId id) const noexcept {
			return id.to_index() < m_components.size() && id.valid();
		}

		[[nodiscard]] size_t size() const noexcept {
			return m_components.size();
		}

		[[nodiscard]] bool empty() const noexcept {
			return m_components.empty();
		}

	private:
		ComponentInfos m_components;
		ComponentIdTypeInfoMap m_type_info_map;
	};
}
