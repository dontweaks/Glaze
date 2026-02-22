#pragma once

#include "ECS/Archetype/Archetype.h"

#include "BundleMeta.h"

namespace glaze::ecs {
	struct BundleManager {
		BundleManager() noexcept = default;

		BundleManager(const BundleManager& other) = delete;
		BundleManager& operator=(const BundleManager& other) = delete;

		BundleManager(BundleManager&& other) = delete;
		BundleManager& operator=(BundleManager&& other) = delete;

		template<Bundle B>
		BundleId register_bundle(ComponentManager& component_manager) {
			using U = std::remove_cvref_t<B>;
			constexpr auto type_id = utils::type_id_ct<BundleKeyType<U>>();
			const auto it = m_bundle_map.find(type_id);
			if (it != m_bundle_map.end()) {
				return it->second;
			}

			const auto bundle_id = BundleId::from_index(m_bundles.size());
			m_bundles.push_back(BundleMeta::create<U>(bundle_id, component_manager));
			m_bundle_map.emplace(type_id, bundle_id);
			return bundle_id;
		}

		[[nodiscard]] std::span<const BundleMeta> bundles() const noexcept { return m_bundles; }

		template<Bundle B>
		[[nodiscard]] BundleId bundle_id() const noexcept {
			using U = std::remove_cvref_t<B>;
			return bundle_id(utils::TypeInfo::of<BundleKeyType<U>>());
		}

		[[nodiscard]] BundleId bundle_id(const utils::TypeInfo& type_info) const noexcept {
			const auto it = m_bundle_map.find(type_info);
			if (it == m_bundle_map.end()) {
				return utils::null_id;
			}
			return it->second;
		}

		[[nodiscard]] std::span<const StorageType> storage_types(const BundleId id) const noexcept {
			const auto index = id.to_index();
			if (index >= m_bundles.size()) {
				utils::panic("Bundle id {} is out of range", id.get());
			}
			return m_bundles[index].storages();
		}

		[[nodiscard]] auto& operator[](this auto& self, const BundleId id) noexcept {
			return self.m_bundles[id.to_index()];
		}

		[[nodiscard]] auto& at(this auto& self, const BundleId id) noexcept {
			const auto index = id.to_index();
			if (index >= self.m_bundles.size()) {
				utils::panic("Bundle id {} is out of range", id.get());
			}

			return self.m_bundles.at(index);
		}

		[[nodiscard]] size_t size() const noexcept { return m_bundles.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_bundles.empty(); }

	private:
		std::vector<BundleMeta> m_bundles;
		utils::TypeInfoMap<BundleId> m_bundle_map;
	};
}