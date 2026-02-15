#pragma once

#include <array>
#include <bit>
#include <memory>
#include <bitset>
#include <optional>
#include <cassert>
#include <vector>

#include "Utils/SwapRemove.h"

namespace glaze::ecs {
	template<typename I>
	concept SparseIndex =
		requires (I i) {
			{ i.to_index() } noexcept -> std::same_as<size_t>;
		};

	template<SparseIndex I, std::move_constructible V, size_t PAGE_SIZE = 4096> requires (PAGE_SIZE > 0)
	struct SparseArray {
		SparseArray() = default;

		SparseArray(const SparseArray&) = delete;
		SparseArray& operator=(const SparseArray&) = delete;

		SparseArray(SparseArray&&) noexcept = default;
		SparseArray& operator=(SparseArray&&) noexcept = default;

		template<typename... Args> requires std::constructible_from<V, Args...>
		V& emplace(const I index, Args&&... args) {
			const size_t pos = index.to_index();
			Page& page = ensure_page(page_index(pos));
			const size_t off = page_offset(pos);

			const bool existed = page.contains(off);
			V& value = page.emplace(off, std::forward<Args>(args)...);

			if (!existed) {
				++m_live;
			}

			return value;
		}

		void insert(const I index, const V& value) {
			emplace(index, value);
		}

		void insert(const I index, V&& value) {
			emplace(index, std::move(value));
		}

		std::optional<V> remove(const I index) noexcept(std::is_nothrow_move_constructible_v<V>) {
			const size_t pos = index.to_index();
			const size_t pi = page_index(pos);

			Page* page = try_page(pi);
			if (!page) {
				return std::nullopt;
			}

			const size_t off = page_offset(pos);
			auto out = page->erase(off);
			if (!out) {
				return std::nullopt;
			}

			--m_live;

			if (page->empty()) {
				m_pages[pi].reset();
				trim_trailing_empty_pages();
			}

			return out;
		}

		void reserve(const size_t span) {
			m_pages.reserve(pages_for_span(span));
		}

		[[nodiscard]] std::optional<std::reference_wrapper<V>> at(const I index) noexcept {
			const size_t pos = index.to_index();
			Page* page = try_page(page_index(pos));
			if (!page) {
				return std::nullopt;
			}

			const size_t off = page_offset(pos);
			if (!page->contains(off)) {
				return std::nullopt;
			}

			return std::optional<std::reference_wrapper<V>>{std::ref(page->get(off))};
		}

		[[nodiscard]] std::optional<std::reference_wrapper<const V>> at(const I index) const noexcept {
			const size_t pos = index.to_index();
			const Page* page = try_page(page_index(pos));
			if (!page) {
				return std::nullopt;
			}

			const size_t off = page_offset(pos);
			if (!page->contains(off)) {
				return std::nullopt;
			}

			return std::optional<std::reference_wrapper<const V>>{std::cref(page->get(off))};
		}

		[[nodiscard]] auto& operator[](this auto& self, const I index) noexcept {
			const size_t pos = index.to_index();
			auto* page = self.try_page(page_index(pos));
			assert(page);
			const size_t off = page_offset(pos);
			assert(page->contains(off));
			return page->get(off);
		}

		[[nodiscard]] bool contains(const I index) const noexcept {
			const size_t pos = index.to_index();
			const Page* page = try_page(page_index(pos));
			return page && page->contains(page_offset(pos));
		}

		[[nodiscard]] bool empty() const noexcept { return m_live == 0; }
		[[nodiscard]] size_t size() const noexcept { return m_live; }

		void clear() noexcept {
			m_pages.clear();
			m_live = 0;
		}

	private:
		struct Page {
			Page() = default;

			Page(const Page&) = delete;
			Page& operator=(const Page&) = delete;

			Page(Page&&) noexcept = default;
			Page& operator=(Page&&) noexcept = default;

			~Page() { clear(); }

			template<typename... Args> requires std::constructible_from<V, Args...>
			V& emplace(const size_t i, Args&&... args) {
				assert(i < PAGE_SIZE);

				if (contains(i)) {
					std::destroy_at(ptr(i));
				} else {
					m_used.set(i);
					++m_live;
				}

				return *std::construct_at(ptr(i), std::forward<Args>(args)...);
			}

			std::optional<V> remove(const size_t i) noexcept(std::is_nothrow_move_constructible_v<V>) {
				assert(i < PAGE_SIZE);

				if (!contains(i)) {
					return std::nullopt;
				}

				std::optional<V> out{std::in_place, std::move(get(i))};
				std::destroy_at(ptr(i));
				m_used.reset(i);
				--m_live;
				return out;
			}

			void clear() noexcept {
				for (size_t i = 0; i < PAGE_SIZE; ++i) {
					if (m_used.test(i)) {
						std::destroy_at(ptr(i));
					}
				}
				m_used.reset();
			}

			[[nodiscard]] bool contains(const size_t i) const noexcept {
				assert(i < PAGE_SIZE);
				return m_used.test(i);
			}

			[[nodiscard]] bool empty() const noexcept {
				return m_live == 0;
			}

			[[nodiscard]] auto& get(this auto& self, const size_t i) noexcept {
				assert(i < PAGE_SIZE);
				return *self.ptr(i);
			}

			[[nodiscard]] auto ptr(this auto& self, const size_t i) noexcept {
				assert(i < PAGE_SIZE);

				using Self = std::remove_reference_t<decltype(self)>;
				using Ptr  = std::conditional_t<std::is_const_v<Self>, const V*, V*>;

				auto* p = self.m_data.data() + i * sizeof(V);
				return std::launder(reinterpret_cast<Ptr>(p));
			}

		private:
			alignas(V) std::array<std::byte, sizeof(V) * PAGE_SIZE> m_data{};
			std::bitset<PAGE_SIZE> m_used{};
			size_t m_live = 0;
		};

		[[nodiscard]] static constexpr size_t pages_for_span(const size_t span) noexcept {
			return span == 0 ? 0 : (span - 1) / PAGE_SIZE + 1;
		}

		[[nodiscard]] static constexpr size_t page_index(const size_t pos) noexcept {
			if constexpr (std::has_single_bit(PAGE_SIZE)) {
				return pos >> std::countr_zero(PAGE_SIZE);
			} else {
				return pos / PAGE_SIZE;
			}
		}

		[[nodiscard]] static constexpr size_t page_offset(const size_t pos) noexcept {
			if constexpr (std::has_single_bit(PAGE_SIZE)) {
				return pos & (PAGE_SIZE - 1);
			} else {
				return pos % PAGE_SIZE;
			}
		}

		[[nodiscard]] auto try_page(this auto& self, const size_t page) noexcept {
			using Self = std::remove_reference_t<decltype(self)>;
			using Ptr = std::conditional_t<std::is_const_v<Self>, const Page*, Page*>;

			if (page >= self.m_pages.size()) {
				return Ptr{nullptr};
			}

			return static_cast<Ptr>(self.m_pages[page].get());
		}

		[[nodiscard]] Page& ensure_page(const size_t page) {
			if (page >= m_pages.size()) {
				m_pages.resize(page + 1);
			}

			if (!m_pages[page]) {
				m_pages[page] = std::make_unique<Page>();
			}

			return *m_pages[page];
		}

		void trim_trailing_empty_pages() noexcept {
			while (!m_pages.empty() && !m_pages.back()) {
				m_pages.pop_back();
			}
		}

		std::vector<std::unique_ptr<Page>> m_pages;
		size_t m_live = 0;
	};

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

		template<typename... Args> requires std::constructible_from<V, Args...> && std::assignable_from<V&, V>
		V& emplace(const I index, Args&&...args) {
			if (const auto dense_index_opt = m_sparse.at(index)) {
				const size_t dense_index = dense_index_opt->get();
				m_dense[dense_index] = V(std::forward<Args>(args)...);
				return m_dense[dense_index];
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
