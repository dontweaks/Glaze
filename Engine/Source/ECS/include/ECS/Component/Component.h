#pragma once

#include <vector>
#include <optional>
#include <unordered_map>

#include "ECS/Ids.h"

#include "Utils/Layout.h"
#include "Utils/TypeOps.h"
#include "Utils/TypeInfo.h"
#include "Utils/HashCombine.h"

namespace glaze::ecs {
	enum struct StorageType : uint8_t {
		Table,
		SparseSet
	};

	template<typename T>
	concept Component = std::is_standard_layout_v<std::remove_cvref_t<T>>;

	template<typename T>
	concept HasStorageType = Component<T> && requires {
		{ std::remove_cvref_t<T>::STORAGE_TYPE } -> std::convertible_to<StorageType>;
	};

	template<Component T>
	[[nodiscard]] consteval StorageType get_storage_type() noexcept {
		using U = std::remove_cvref_t<T>;
		if constexpr (HasStorageType<U>) {
			return U::STORAGE_TYPE;
		} else {
			return StorageType::Table;
		}
	}

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

		[[nodiscard]] const ComponentMeta* get_info(const ComponentId id) const noexcept {
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

		[[nodiscard]] size_t size() const noexcept { return m_components.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_components.empty(); }

	private:
		std::vector<ComponentMeta> m_components;
		utils::TypeInfoMap<ComponentId> m_components_map;
	};
	
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
}
