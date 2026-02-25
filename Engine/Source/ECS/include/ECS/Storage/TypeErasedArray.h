#pragma once
#include <cassert>
#include <span>

#include "Utils/Layout.h"
#include "Utils/TypeOps.h"

namespace glaze::ecs {
	struct TypeErasedArray {
		using Layout = utils::Layout;
		using TypeOps = utils::TypeOps;

		TypeErasedArray(const Layout& layout, const TypeOps& type_ops, const size_t capacity) noexcept
			: m_layout(layout), m_type_ops(type_ops), m_capacity(capacity) {
		}
		TypeErasedArray() = default;

		~TypeErasedArray() {
			for (size_t i = 0; i < m_size; ++i) {
				m_type_ops.destruct(get(i));
			}
			m_size = 0;
		}

		TypeErasedArray(const TypeErasedArray&) = delete;
		TypeErasedArray& operator=(const TypeErasedArray&) = delete;

		TypeErasedArray(TypeErasedArray&& other) noexcept
			: m_layout(std::move(other.m_layout)),
			  m_type_ops(std::move(other.m_type_ops)),
			  m_data(std::exchange(other.m_data, nullptr)),
			  m_size(std::exchange(other.m_size, 0)),
		      m_capacity(std::exchange(other.m_capacity, 0)) {
		}

		TypeErasedArray& operator=(TypeErasedArray&& other) noexcept {
			if (&other != this) {
				m_layout = std::move(other.m_layout);
				m_type_ops = std::move(other.m_type_ops);
				m_data = std::exchange(other.m_data, nullptr);
				m_size = std::exchange(other.m_size, 0);
				m_capacity = std::exchange(other.m_capacity, 0);
			}

			return *this;
		}

		template<typename T>
		[[nodiscard]] std::span<T> get_slice(const size_t index, const size_t length) noexcept {
			assert(index + length <= m_size && "Slice out of bounds");
			return std::span(get<T>(index), length);
		}

		template<typename T>
		[[nodiscard]] std::span<const T> get_slice(const size_t index, const size_t length) const noexcept {
			assert(index + length <= m_size && "Slice out of bounds");
			return std::span(get<T>(index), length);
		}

		template<typename T>
		[[nodiscard]] T* get(const size_t index) noexcept {
			assert(m_layout == Layout::of<T>());
			return std::launder(static_cast<T*>(get(index)));
		}

		template<typename T>
		[[nodiscard]] const T* get(const size_t index) const noexcept {
			assert(m_layout == Layout::of<T>());
			return std::launder(static_cast<const T*>(get(index)));
		}

		[[nodiscard]] void* get(const size_t index) noexcept {
			assert(index < m_size && "Index out of bounds");
			return m_data + index * m_layout.size();
		}

		[[nodiscard]] const void* get(const size_t index) const noexcept {
			assert(index < m_size && "Index out of bounds");
			return m_data + index * m_layout.size();
		}

		void swap_remove(const size_t index_to_remove, const size_t index_to_keep) {
			assert(index_to_remove < m_size && "Index to remove out of bounds");
			assert(index_to_keep < m_size && "Index to keep out of bounds");

			const auto ptr_to_keep = get(index_to_keep);
			const auto ptr_to_remove = get(index_to_remove);

			if (index_to_remove != index_to_keep) {
				m_type_ops.swap(ptr_to_keep, ptr_to_remove, 1);
			}

			m_type_ops.destruct(ptr_to_keep);
		}

		[[nodiscard]] bool is_zst() const noexcept { return m_layout.size() == 0; }
		[[nodiscard]] size_t size() const noexcept { return m_size; }
		[[nodiscard]] size_t capacity() const noexcept { return m_capacity; }
		[[nodiscard]] const Layout& layout() const noexcept { return m_layout; }
		[[nodiscard]] const TypeOps& type_ops() const noexcept { return m_type_ops; }

		[[nodiscard]] std::byte* data() noexcept { return m_data;}
		[[nodiscard]] const std::byte* data() const noexcept { return m_data;}

	private:
		Layout m_layout;
		TypeOps m_type_ops{};
		std::byte* m_data = nullptr;
		size_t m_size = 0;
		size_t m_capacity = 0;
	};
}
