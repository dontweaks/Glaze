#pragma once

#include <cassert>

#include "Utils/Layout.h"
#include "Utils/Panic.h"
#include "Utils/TypeOps.h"

namespace glaze::ecs {
	struct TypeErasedArray {
		using Layout = utils::Layout;
		using TypeOps = utils::TypeOps;

		TypeErasedArray() = default;

		TypeErasedArray(const Layout& layout, const TypeOps& type_ops, const size_t capacity = 0) noexcept
			: m_layout(layout), m_type_ops(type_ops) {
			reserve(capacity);
		}

		~TypeErasedArray() { destroy_and_deallocate(); }

		TypeErasedArray(const TypeErasedArray&) = delete;
		TypeErasedArray& operator=(const TypeErasedArray&) = delete;

		TypeErasedArray(TypeErasedArray&& other) noexcept
			: m_layout(std::exchange(other.m_layout, {})),
			  m_type_ops(std::exchange(other.m_type_ops, {})),
			  m_data(std::exchange(other.m_data, nullptr)),
			  m_size(std::exchange(other.m_size, 0)),
			  m_capacity(std::exchange(other.m_capacity, 0)) {
		}

		TypeErasedArray& operator=(TypeErasedArray&& other) noexcept {
			if (this != &other) {
				destroy_and_deallocate();

				m_layout   = std::exchange(other.m_layout, {});
				m_type_ops = std::exchange(other.m_type_ops, {});
				m_data     = std::exchange(other.m_data, nullptr);
				m_size     = std::exchange(other.m_size, 0);
				m_capacity = std::exchange(other.m_capacity, 0);
			}
			return *this;
		}

		template<typename T, typename ... Args>
		T& emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<T>, Args...>) {
			assert(m_layout == Layout::of<T>());

			auto construct = [&](void* const p) { std::construct_at(static_cast<T*>(p), std::forward<Args>(args)...); };

			if (void* const slot = emplace_back_untyped(construct)) {
				return *std::launder(static_cast<T*>(slot));
			}

			return *get<T>(m_size - 1);
		}

		template<typename T>
		void push_back(const T& v) noexcept(std::is_nothrow_copy_constructible_v<std::remove_cvref_t<T>>) {
			assert(m_layout == Layout::of<T>());

			emplace_back_untyped([&](void* const p) noexcept {
				m_type_ops.copy_construct(p, std::addressof(v));
			});
		}

		template<typename T> requires (!std::is_lvalue_reference_v<T>)
		void push_back(T&& v) noexcept {
			assert(m_layout == Layout::of<T>());

			emplace_back_untyped([&](void* const p) noexcept {
				m_type_ops.move_construct(p, std::addressof(v));
			});
		}

		template<typename T>
		void insert(const size_t index, const T& v) {
			assert(m_layout == Layout::of<T>());
			assert(index <= m_size && "Index out of bounds");
			if (index == m_size) {
				push_back(v);
			} else {
				replace(index, v);
			}
		}

		template<typename T> requires (!std::is_lvalue_reference_v<T>)
		void insert(const size_t index, T&& v) noexcept {
			assert(m_layout == Layout::of<T>());
			assert(index <= m_size && "Index out of bounds");
			if (index == m_size) {
				push_back(std::forward<T>(v));
			} else {
				replace(index, std::forward<T>(v));
			}
		}

		template<typename T>
		void append_move(const std::span<T> src) noexcept {
			assert(m_layout == Layout::of<T>());

			if (src.empty()) return;

			if (zst()) {
				ensure_capacity_for(src.size());
				m_size += src.size();
				return;
			}

			ensure_capacity_for(src.size());

			for (T& v : src) {
				m_type_ops.move_construct(get(m_size), std::addressof(v));
				++m_size;
			}
		}

		template<typename T>
		[[nodiscard]] T* get(const size_t index) noexcept {
			assert(index < m_size && "Index out of bounds");
			assert(m_layout == Layout::of<T>());
			if (zst()) {
				return zst_dummy<T>();
			}
			return std::launder(static_cast<T*>(get(index)));
		}

		template<typename T>
		[[nodiscard]] const T* get(const size_t index) const noexcept {
			assert(index < m_size && "Index out of bounds");
			assert(m_layout == Layout::of<T>());
			if (zst()) {
				return zst_dummy<T>();
			}
			return std::launder(static_cast<const T*>(get(index)));
		}

		template<typename T>
		[[nodiscard]] std::span<T> get_slice(const size_t index, const size_t length) noexcept {
			assert(!zst() && "Slices for ZST are meaningless");
			assert(index + length <= m_size && "Slice out of bounds");
			return std::span(get<T>(index), length);
		}

		template<typename T>
		[[nodiscard]] std::span<const T> get_slice(const size_t index, const size_t length) const noexcept {
			assert(!zst() && "Slices for ZST are meaningless");
			assert(index + length <= m_size && "Slice out of bounds");
			return std::span(get<T>(index), length);
		}

		template<typename T>
		void replace(const size_t index, const T& v) noexcept(std::is_nothrow_copy_assignable_v<std::remove_cvref_t<T>>) {
			assert(index < m_size && "Index out of bounds");
			copy_replace(index, std::addressof(v));
		}

		template<typename T> requires (!std::is_lvalue_reference_v<T>)
		void replace(const size_t index, T&& v) noexcept {
			assert(index < m_size && "Index out of bounds");
			move_replace(index, std::addressof(v));
		}

		template<typename T>
		T swap_remove(const size_t index) noexcept {
			assert(m_layout == Layout::of<T>());
			assert(index < m_size);

			if (zst()) {
				swap_remove(index);
				return T{};
			}

			T out = std::move(*get<T>(index));
			swap_remove(index);
			return out;
		}

		template<typename Init>
		void* emplace_back_untyped(Init&& init) {
			if (zst()) {
				ensure_capacity_for(1);
				++m_size;
				return nullptr;
			}

			ensure_capacity_for(1);
			void* const slot = get(m_size);
			std::forward<Init>(init)(slot);
			++m_size;
			return slot;
		}

		void* move_emplace_back(void* const v) {
			if (zst()) {
				ensure_capacity_for(1);
				++m_size;
				return nullptr;
			}

			ensure_capacity_for(1);
			void* const slot = get(m_size);
			m_type_ops.move_construct(slot, v);
			++m_size;
			return slot;
		}

		[[nodiscard]] void* get(const size_t index) noexcept {
			if (zst()) {
				return nullptr;
			}
			assert(index < m_capacity && "Index out of capacity bounds");
			assert(m_data && "No backing storage");
			return m_data + index * m_layout.size();
		}

		[[nodiscard]] const void* get(const size_t index) const noexcept {
			if (zst()) {
				return nullptr;
			}
			assert(index < m_capacity && "Index out of capacity bounds");
			assert(m_data && "No backing storage");
			return m_data + index * m_layout.size();
		}

		void reserve(const size_t new_capacity) noexcept {
			if (zst()) {
				m_capacity = std::max(m_capacity, new_capacity);
				return;
			}

			if (new_capacity <= m_capacity) {
				return;
			}

			std::byte* new_data = allocate_bytes(new_capacity);
			assert(new_data && "Allocation failed");

			for (size_t i = 0; i < m_size; ++i) {
				m_type_ops.move_construct(new_data + i * m_layout.size(), get(i));
				m_type_ops.destruct(get(i));
			}

			deallocate_bytes(m_data);
			m_data = new_data;
			m_capacity = new_capacity;
		}

		void resize(const size_t new_size) {
			if (new_size == m_size) {
				return;
			}

			if (zst()) {
				m_size = new_size;
				m_capacity = std::max(m_capacity, new_size);
				return;
			}

			if (new_size < m_size) {
				for (size_t i = new_size; i < m_size; ++i) {
					m_type_ops.destruct(get(i));
				}
				m_size = new_size;
				return;
			}

			ensure_capacity_for(new_size - m_size);

			assert(m_type_ops.construct && "Not default-constructible: use resize(new_size, init) or emplace_back");
			for (size_t i = m_size; i < new_size; ++i) {
				m_type_ops.construct(get(i));
			}
			m_size = new_size;
		}

		template<typename Init>
		void resize(const size_t new_size, Init&& init) {
			if (new_size == m_size) {
				return;
			}

			if (zst()) {
				m_size = new_size;
				m_capacity = std::max(m_capacity, new_size);
				return;
			}

			if (new_size < m_size) {
				for (size_t i = new_size; i < m_size; ++i) {
					m_type_ops.destruct(get(i));
				}
				m_size = new_size;
				return;
			}

			ensure_capacity_for(new_size - m_size);

			for (size_t i = m_size; i < new_size; ++i) {
				std::forward<Init>(init)(get(i), i);
				++m_size;
			}
		}

		void copy_replace(const size_t index, const void* const value) {
			assert(index < m_size && "Index out of bounds");
			m_type_ops.copy_assign(get(index), value);
		}

		void move_replace(const size_t index, void* const value) noexcept {
			assert(index < m_size && "Index out of bounds");
			m_type_ops.move_assign(get(index), value);
		}

		void swap_remove(const size_t index_to_remove, const size_t index_to_keep) noexcept {
			assert(index_to_remove < m_size && "Index to remove out of bounds");
			assert(index_to_keep < m_size && "Index to keep out of bounds");

			if (zst()) {
				--m_size;
				return;
			}

			void* const ptr_to_keep = get(index_to_keep);
			void* const ptr_to_remove = get(index_to_remove);

			if (index_to_remove != index_to_keep) {
				m_type_ops.move_assign(ptr_to_remove, ptr_to_keep);
			}

			m_type_ops.destruct(ptr_to_keep);

			--m_size;
		}

		void swap_remove(const size_t index) noexcept {
			swap_remove(index, m_size - 1);
		}

		[[nodiscard]] bool zst() const noexcept { return m_layout.size() == 0; }
		[[nodiscard]] size_t size() const noexcept { return m_size; }
		[[nodiscard]] size_t capacity() const noexcept { return m_capacity; }
		[[nodiscard]] bool empty() const noexcept { return m_size == 0; }
		[[nodiscard]] const Layout& layout() const noexcept { return m_layout; }
		[[nodiscard]] const TypeOps& type_ops() const noexcept { return m_type_ops; }

		[[nodiscard]] std::byte* data() noexcept { return m_data; }
		[[nodiscard]] const std::byte* data() const noexcept { return m_data; }

	private:
		template<std::default_initializable T>
		[[nodiscard]] static T* zst_dummy() noexcept {
			using U = std::remove_cvref_t<T>;
			static U dummy{};
			return std::addressof(dummy);
		}

		void ensure_capacity_for(const size_t additional) noexcept {
			const size_t needed = m_size + additional;

			if (zst()) {
				m_capacity = std::max(m_capacity, needed);
				return;
			}

			if (needed <= m_capacity) return;

			const size_t grown = m_capacity == 0 ? std::max<size_t>(needed, 8u) : std::max<size_t>(needed, m_capacity * 2u);
			reserve(grown);
		}

		[[nodiscard]] std::byte* allocate_bytes(const size_t capacity) const noexcept {
			const size_t bytes = capacity * m_layout.size();
			const auto data = static_cast<std::byte*>(operator new(bytes, static_cast<std::align_val_t>(m_layout.align()), std::nothrow));
			if (!data) [[unlikely]] {
				utils::panic("TypeErasedArray: allocation of {} bytes failed", capacity * m_layout.size());
			}
			return data;
		}

		void deallocate_bytes(std::byte* ptr) const noexcept {
			if (!ptr) {
				return;
			}
			operator delete(ptr, static_cast<std::align_val_t>(m_layout.align()));
		}

		void destroy_and_deallocate() noexcept {
			if (!zst() && m_data) {
				for (size_t i = 0; i < m_size; ++i) {
					m_type_ops.destruct(get(i));
				}
				deallocate_bytes(m_data);
			}
			m_data = nullptr;
			m_size = 0;
			m_capacity = 0;
		}

		Layout m_layout;
		TypeOps m_type_ops{};
		std::byte* m_data = nullptr;
		size_t m_size = 0;
		size_t m_capacity = 0;
	};
}