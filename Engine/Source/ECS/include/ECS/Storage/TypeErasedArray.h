#pragma once

#include "Utils/Layout.h"
#include "Utils/TypeOps.h"

namespace glaze::ecs {
	struct TypeErasedArray {
		using Layout = utils::Layout;
		using TypeOps = utils::TypeOps;

		TypeErasedArray() = default;

		TypeErasedArray(const Layout& layout, const TypeOps& type_ops, const size_t capacity) noexcept
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

			if (zst()) {
				ensure_capacity_for(1);
				++m_size;
				return *get<T>(m_size - 1);
			}

			ensure_capacity_for(1);
			auto* p = std::construct_at(static_cast<T*>(get(m_size)), std::forward<Args>(args)...);
			++m_size;
			return *std::launder(p);
		}

		template<typename T>
		T& push_back(T&& v) noexcept {
			using U = std::remove_cvref_t<T>;
			assert(m_layout == Layout::of<U>());

			if (zst()) {
				ensure_capacity_for(1);
				++m_size;
				return *get<U>(m_size - 1);
			}

			ensure_capacity_for(1);

			m_type_ops.move_construct(get(m_size), std::addressof(v));
			++m_size;
			return *get<U>(m_size - 1);
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

		template<typename Init>
		void* emplace_back_untyped(Init&& init) {
			if (zst()) {
				ensure_capacity_for(1);
				++m_size;
				return nullptr;
			}

			ensure_capacity_for(1);
			std::forward<Init>(init)(get(m_size));
			++m_size;
			return get(m_size - 1);
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

		void swap_remove(const size_t index_to_remove, const size_t index_to_keep) {
			assert(index_to_remove < m_size && "Index to remove out of bounds");
			assert(index_to_keep < m_size && "Index to keep out of bounds");

			if (zst()) {
				--m_size;
				return;
			}

			const auto ptr_to_keep = get(index_to_keep);
			const auto ptr_to_remove = get(index_to_remove);

			if (index_to_remove != index_to_keep) {
				// m_type_ops.swap(ptr_to_keep, ptr_to_remove, 1);
			}

			m_type_ops.destruct(ptr_to_keep);
			--m_size;
		}

		[[nodiscard]] bool zst() const noexcept { return m_layout.size() == 0; }
		[[nodiscard]] size_t size() const noexcept { return m_size; }
		[[nodiscard]] size_t capacity() const noexcept { return m_capacity; }
		[[nodiscard]] const Layout& layout() const noexcept { return m_layout; }
		[[nodiscard]] const TypeOps& type_ops() const noexcept { return m_type_ops; }

		[[nodiscard]] std::byte* data() noexcept { return m_data; }
		[[nodiscard]] const std::byte* data() const noexcept { return m_data; }

	private:
		template<std::default_initializable T>
		static T* zst_dummy() noexcept {
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
			return static_cast<std::byte*>(operator new(bytes, static_cast<std::align_val_t>(m_layout.align()), std::nothrow));
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