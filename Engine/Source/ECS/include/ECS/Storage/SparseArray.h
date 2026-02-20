#pragma once

#include <bit>
#include <bitset>
#include <array>
#include <vector>
#include <memory>
#include <cassert>
#include <optional>

#include "SparseIndex.h"

namespace glaze::ecs {
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
			auto out = page->remove(off);
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
		[[nodiscard]] size_t page_count() const noexcept { return m_pages.size(); }

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
}