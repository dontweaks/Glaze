#pragma once

#include <vector>
#include <optional>
#include <unordered_map>

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
		{ T::STORAGE_TYPE } -> std::convertible_to<StorageType>;
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

	struct [[nodiscard]] ComponentDesc {
		template<Component T>
		static consteval ComponentDesc of() noexcept {
			return ComponentDesc {
				.m_name = utils::TypeName<T>(),
				.m_storage_type = get_storage_type<T>(),
				.m_layout = utils::Layout::of<T>(),
				.m_type_ops = utils::TypeOps::of<T>(),
				.m_type_info = utils::TypeInfo::of<T>()
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

		constexpr StorageType storage_type() const noexcept { return m_storage_type; }
		constexpr const utils::TypeInfo& type_info() const noexcept { return m_type_info; }
		constexpr std::string_view name() const noexcept { return m_name; }

	private:
		friend struct ComponentInfo;

		std::string_view m_name;
		StorageType m_storage_type = StorageType::Table;
		utils::Layout m_layout;
		utils::TypeOps m_type_ops;
		utils::TypeInfo m_type_info{};
	};

	struct [[nodiscard]] ComponentInfo {
		constexpr ComponentInfo(const ComponentId id, ComponentDesc desc)
			: m_id(id), m_desc(std::move(desc)) {
		}

		constexpr ComponentId id() const noexcept { return m_id; }
		constexpr std::string_view name() const noexcept { return m_desc.m_name; }
		constexpr StorageType storage_type() const noexcept { return m_desc.m_storage_type; }
		constexpr const utils::Layout& layout() const noexcept { return m_desc.m_layout; }
		constexpr const utils::TypeInfo& type_info() const noexcept { return m_desc.m_type_info; }
		constexpr const utils::TypeOps& type_ops() const noexcept { return m_desc.m_type_ops; }

	private:
		friend struct ComponentManager;

		ComponentId m_id;
		ComponentDesc m_desc;
	};

	using ComponentInfos = std::vector<ComponentInfo>;
	using ComponentIdTypeInfoMap = std::unordered_map<utils::TypeInfo, ComponentId>;

	struct [[nodiscard]] ComponentManager {
		template<Component T>
		ComponentId component_id() const noexcept {
			return get_id(utils::TypeInfo::of<T>());
		}

		ComponentId get_id(const utils::TypeInfo& type_info) const noexcept {
			const auto it = m_type_info_map.find(type_info);
			if (it == m_type_info_map.end()) {
				return ComponentId::INVALID_VALUE;
			}
			return it->second;
		}

		const ComponentInfo* get_info(const ComponentId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_components.size()) {
				return nullptr;
			}

			return &m_components[index];
		}

		const ComponentDesc* get_desc(const ComponentId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_components.size()) {
				return nullptr;
			}
			return &m_components[index].m_desc;
		}

		std::optional<std::string_view> get_name(const ComponentId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_components.size()) {
				return std::nullopt;
			}
			return m_components[index].m_desc.name();
		}

		bool is_id_valid(const ComponentId id) const noexcept {
			return id.to_index() < m_components.size();
		}

		size_t size() const noexcept {
			return m_components.size();
		}

		bool empty() const noexcept {
			return m_components.empty();
		}

	private:
		void register_component(const ComponentId id, ComponentInfo desc) {
			const auto index = id.to_index();
			if (m_components.size() <= index) {
				m_components.push_back(std::move(desc));
			} else {
				m_components[index] = desc;
			}
		}

		ComponentInfos m_components;
		ComponentIdTypeInfoMap m_type_info_map;
	};
}
