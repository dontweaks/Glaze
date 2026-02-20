#pragma once

#include <ranges>

#include "SparseArray.h"

#include "Utils/SwapRemove.h"

namespace glaze::ecs {
	template<SparseIndex I, std::move_constructible V, size_t PAGE_SIZE = 4096> requires (PAGE_SIZE > 0)
	struct SparseSet {
		SparseSet() = default;
		explicit SparseSet(const size_t capacity) {
			m_dense.reserve(capacity);
			m_indices.reserve(capacity);
		}

		SparseSet(const SparseSet&) = delete;
		SparseSet& operator=(const SparseSet&) = delete;

		SparseSet(SparseSet&&) noexcept = default;
		SparseSet& operator=(SparseSet&&) noexcept = default;

		template<typename... Args> requires std::constructible_from<V, Args...>
		V& emplace(const I index, Args&&...args) {
			if (const auto dense_index_opt = m_sparse.at(index)) {
				const size_t dense_index = dense_index_opt->get();
				std::destroy_at(&m_dense[dense_index]);
				return *std::construct_at(&m_dense[dense_index], std::forward<Args>(args)...);
			}

			const size_t dense_index = m_dense.size();
			m_sparse.emplace(index, dense_index);
			m_indices.push_back(index);
			m_dense.emplace_back(std::forward<Args>(args)...);
			return m_dense.back();
		}

		void insert(const I index, const V& value) {
			emplace(index, value);
		}

		void insert(const I index, V&& value) {
			emplace(index, std::move(value));
		}

		std::optional<V> remove(const I index) noexcept(std::is_nothrow_move_constructible_v<V>) {
			return m_sparse.remove(index).and_then([this](const size_t dense_index) {
				const bool is_last = dense_index == m_dense.size() - 1;
				std::optional value{utils::swap_remove(m_dense, dense_index)};
				utils::swap_remove(m_indices, dense_index);
				if (!is_last) {
					const I moved_index = m_indices[dense_index];
					m_sparse[moved_index] = dense_index;
				}
				return value;
			});
		}

		[[nodiscard]] std::optional<std::reference_wrapper<V>> at(const I index) noexcept {
			return m_sparse.at(index).transform([this](const size_t dense_index) {
				return std::reference_wrapper<V>{m_dense[dense_index]};
			});
		}

		[[nodiscard]] std::optional<std::reference_wrapper<const V>> at(const I index) const noexcept {
			return m_sparse.at(index).transform([this](const size_t dense_index) {
				return std::reference_wrapper<const V>{m_dense[dense_index]};
			});
		}

		[[nodiscard]] auto& operator[](this auto& self, const I index) noexcept {
			const size_t dense_index = self.m_sparse[index];
			return self.m_dense[dense_index];
		}

		[[nodiscard]] auto iter(this auto& self) noexcept {
			return std::views::zip(std::as_const(self.m_indices), self.m_dense);
		}

		[[nodiscard]] const std::vector<I>& indices() const noexcept { return m_indices; }
		[[nodiscard]] auto& values(this auto& self) noexcept { return self.m_dense; }

		[[nodiscard]] bool contains(const I index) const noexcept { return m_sparse.contains(index); }
		[[nodiscard]] bool empty() const noexcept { return m_dense.empty(); }

		[[nodiscard]] size_t size() const noexcept { return m_dense.size(); }
		[[nodiscard]] size_t capacity() const noexcept { return m_dense.capacity(); }
		[[nodiscard]] size_t page_count() const noexcept { return m_sparse.page_count(); }

		void reserve(const size_t cap) {
			m_dense.reserve(cap);
			m_indices.reserve(cap);
			m_sparse.reserve(cap);
		}

		void clear() {
			m_dense.clear();
			m_indices.clear();
			m_sparse.clear();
		}

	private:
		std::vector<V> m_dense;
		std::vector<I> m_indices;
		SparseArray<I, size_t, PAGE_SIZE> m_sparse;
	};
}
