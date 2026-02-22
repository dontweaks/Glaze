#pragma once

#include "Utils/Layout.h"
#include "Utils/TypeOps.h"
#include "Utils/TypeInfo.h"

#include "Component.h"

namespace glaze::ecs {
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
		friend struct ComponentMeta;

		std::string_view m_name;
		StorageType m_storage_type = StorageType::Table;
		utils::Layout m_layout;
		utils::TypeOps m_type_ops;
		utils::TypeInfo m_type_info{};
	};

	struct ComponentMeta {
		ComponentMeta(const ComponentId id, const ComponentDesc& desc) noexcept
			: m_id(id), m_desc(desc) {
		}

		ComponentMeta(const ComponentMeta& other) = delete;
		ComponentMeta& operator=(const ComponentMeta& other) = delete;

		ComponentMeta(ComponentMeta&& other) noexcept = default;
		ComponentMeta& operator=(ComponentMeta&& other) noexcept = default;

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
}