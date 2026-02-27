#include <gtest/gtest.h>
#include <array>

#include "ECS/Storage/TypeErasedArray.h"

#include <iostream>

namespace glaze::ecs::tests {
	struct TestComponent {
		TestComponent(int x) : x(x) {}
		TestComponent() {}
		~TestComponent() {}

		TestComponent(const TestComponent& other) : x(other.x) {}

		TestComponent(TestComponent&& other) noexcept : x(other.x) {
			other.x = -1;
		}

		TestComponent& operator=(const TestComponent& other) {
			x = other.x;
			return *this;
		}

		TestComponent& operator=(TestComponent&& other) noexcept {
			x = other.x;
			other.x = -1;
			return *this;
		}

		int x = 0;
	};

	struct TagComponent {};

	TEST(TypeErasedArray, Empty) {
		TypeErasedArray array;

		ASSERT_EQ(array.size(), 0);
		ASSERT_EQ(array.capacity(), 0);
		ASSERT_EQ(array.data(), nullptr);
	}

	TEST(TypeErasedArray, ParamConstruct) {
		static constexpr size_t CAPACITY = 64;
		TypeErasedArray array(utils::Layout::of<TestComponent>(),utils::TypeOps::of<TestComponent>(), CAPACITY);

		ASSERT_EQ(array.size(), 0);
		ASSERT_EQ(array.capacity(), CAPACITY);
		ASSERT_EQ(array.zst(), false);
		ASSERT_NE(array.data(), nullptr);
	}

	TEST(TypeErasedArray, Reserve) {
		static constexpr size_t CAPACITY = 64;
		TypeErasedArray array(utils::Layout::of<TestComponent>(), utils::TypeOps::of<TestComponent>());
		array.reserve(CAPACITY);

		ASSERT_EQ(array.size(), 0);
		ASSERT_EQ(array.capacity(), CAPACITY);
		ASSERT_EQ(array.zst(), false);
		ASSERT_NE(array.data(), nullptr);
	}

	TEST(TypeErasedArray, Resize) {
		static constexpr size_t SIZE = 4;
		TypeErasedArray array(utils::Layout::of<TestComponent>(), utils::TypeOps::of<TestComponent>());
		array.resize(4);

		ASSERT_EQ(array.size(), SIZE);
		ASSERT_EQ(array.capacity(), SIZE * 2);
		ASSERT_EQ(array.zst(), false);
		ASSERT_NE(array.data(), nullptr);
	}

	TEST(TypeErasedArray, Emplace) {
		TypeErasedArray array(utils::Layout::of<TestComponent>(), utils::TypeOps::of<TestComponent>(), 64);

		const auto& ref1 = array.emplace_back<TestComponent>(10);
		const auto& ref2 = array.emplace_back<TestComponent>(20);

		EXPECT_EQ(array.size(), 2);
		EXPECT_EQ(ref1.x, 10);
		EXPECT_EQ(ref2.x, 20);

		const auto* ptr1 = array.get<TestComponent>(0);
		const auto* ptr2 = array.get<TestComponent>(1);

		ASSERT_NE(ptr1, nullptr);
		ASSERT_NE(ptr2, nullptr);
		EXPECT_EQ(ptr1->x, 10);
		EXPECT_EQ(ptr2->x, 20);
	}

	TEST(TypeErasedArray, PushBack) {
		TypeErasedArray array(utils::Layout::of<TestComponent>(), utils::TypeOps::of<TestComponent>(), 64);

		TestComponent comp{42};
		array.push_back(comp);

		EXPECT_EQ(array.size(), 1);
		EXPECT_EQ(array.get<TestComponent>(0)->x, 42);
		EXPECT_EQ(comp.x, 42);

		array.push_back(std::move(comp));

		EXPECT_EQ(array.size(), 2);
		EXPECT_EQ(array.get<TestComponent>(0)->x, 42);
		EXPECT_EQ(comp.x, -1);
	}

	TEST(TypeErasedArray, AppendMove) {
		TypeErasedArray array(utils::Layout::of<TestComponent>(), utils::TypeOps::of<TestComponent>(), 64);

		std::vector<TestComponent> comps = { 1, 2, 3 };
		array.append_move(std::span{comps});

		EXPECT_EQ(array.size(), 3);
		EXPECT_EQ(array.get<TestComponent>(0)->x, 1);
		EXPECT_EQ(array.get<TestComponent>(1)->x, 2);
		EXPECT_EQ(array.get<TestComponent>(2)->x, 3);
	}

	TEST(TypeErasedArray, GetSlice) {
		TypeErasedArray array(utils::Layout::of<TestComponent>(), utils::TypeOps::of<TestComponent>(), 64);

		std::vector<TestComponent> comps = { 1, 2, 3 };
		array.append_move(std::span{comps});

		const auto slice = array.get_slice<TestComponent>(0, 3);
		EXPECT_EQ(slice.size(), 3);
		EXPECT_EQ(slice[0].x, 1);
		EXPECT_EQ(slice[1].x, 2);
		EXPECT_EQ(slice[2].x, 3);
	}

	TEST(TypeErasedArray, ZeroSizedTypeHandling) {
		TypeErasedArray array(utils::Layout::of<TagComponent>(), utils::TypeOps::of<TagComponent>(), 64);

		EXPECT_TRUE(array.zst());

		array.emplace_back<TagComponent>();
		array.emplace_back<TagComponent>();

		EXPECT_EQ(array.size(), 2);
		EXPECT_EQ(array.data(), nullptr);

		// Getting a ZST should return a valid dummy pointer, not nullptr
		const auto* ptr = array.get<TagComponent>(0);
		EXPECT_NE(ptr, nullptr);
	}

	TEST(TypeErasedArrayUntyped, Get) {
		static constexpr size_t CAPACITY = 64;
		TypeErasedArray array(utils::Layout::of<TestComponent>(), utils::TypeOps::of<TestComponent>(), CAPACITY);

		const void* data_0 = array.get(0);
		const void* data_1 = array.get(15);
		const void* data_2 = array.get(31);
		const void* data_3 = array.get(63);

		ASSERT_EQ(array.size(), 0);
		ASSERT_EQ(array.capacity(), CAPACITY);
		ASSERT_EQ(array.zst(), false);
		ASSERT_NE(data_0, nullptr);
		ASSERT_NE(data_1, nullptr);
		ASSERT_NE(data_2, nullptr);
		ASSERT_NE(data_3, nullptr);
	}
}