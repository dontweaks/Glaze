#include <gtest/gtest.h>
#include <string>
#include <algorithm>

#include "ECS/Storage/SparseSet.h"

namespace glaze::ecs::tests {
	struct TestIndex {
		size_t value = 0;
		[[nodiscard]] constexpr size_t to_index() const noexcept { return value; }
		[[nodiscard]] constexpr auto operator<=>(const TestIndex& index) const noexcept = default;
	};

	struct SparseSetTest : testing::Test {
	protected:
		SparseSet<TestIndex, int> simple_set;
		SparseSet<TestIndex, std::shared_ptr<int>> complex_set;
		TestIndex index_0{0};
		TestIndex index_1{1};
		static inline int TEST_EXPECTED_VALUE{10};
		static inline int TEST_EXPECTED_VALUE_OVERRIDE{20};
	};

	TEST_F(SparseSetTest, Empty) {
		EXPECT_EQ(simple_set.size(), 0);
		EXPECT_EQ(simple_set.page_count(), 0);
		EXPECT_TRUE(simple_set.empty());
	}

	TEST_F(SparseSetTest, ConstructWithCapacity) {
		constexpr size_t expected_capacity = 1024;
		simple_set = SparseSet<TestIndex, int>(expected_capacity);
		EXPECT_EQ(simple_set.size(), 0);
		EXPECT_EQ(simple_set.capacity(), expected_capacity);
		EXPECT_EQ(simple_set.page_count(), 0);
		EXPECT_TRUE(simple_set.empty());
	}

	TEST_F(SparseSetTest, Emplace) {
		{
			const auto& value = simple_set.emplace(index_0, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
			EXPECT_EQ(simple_set.size(), 1);
			EXPECT_EQ(simple_set.page_count(), 1);
			EXPECT_FALSE(simple_set.empty());
		}

		{
			const auto& value = simple_set.emplace(index_1, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
			EXPECT_EQ(simple_set.size(), 2);
			EXPECT_EQ(simple_set.page_count(), 1);
			EXPECT_FALSE(simple_set.empty());
		}
	}

	TEST_F(SparseSetTest, EmplaceOverride) {
		{
			const auto& value = simple_set.emplace(index_0, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
		}
		{
			const auto& value_overriden = simple_set.emplace(index_0, TEST_EXPECTED_VALUE_OVERRIDE);
			EXPECT_EQ(value_overriden, TEST_EXPECTED_VALUE_OVERRIDE);
		}
		EXPECT_EQ(simple_set.size(), 1);
		EXPECT_FALSE(simple_set.empty());
	}

	TEST_F(SparseSetTest, Insert) {
		auto ptr = std::make_shared<int>();
		complex_set.insert(index_0, ptr);
		EXPECT_EQ(ptr.use_count(), 2);

		complex_set.insert(index_1, std::move(ptr));
		EXPECT_EQ(ptr.use_count(), 0);
		EXPECT_FALSE(ptr);
	}

	TEST_F(SparseSetTest, Remove) {
		simple_set.insert(index_0, TEST_EXPECTED_VALUE);
		simple_set.insert(index_1, TEST_EXPECTED_VALUE);

		const auto rem_result = simple_set.remove(index_0);
		EXPECT_TRUE(rem_result.has_value());
		EXPECT_EQ(rem_result.value(), TEST_EXPECTED_VALUE);

		EXPECT_EQ(simple_set.size(), 1);
		EXPECT_TRUE(simple_set.contains(index_1));
		EXPECT_FALSE(simple_set.contains(index_0));

		const auto rem_result_2 = simple_set.remove(index_1);
		EXPECT_TRUE(rem_result_2.has_value());
		EXPECT_EQ(rem_result_2.value(), TEST_EXPECTED_VALUE);

		EXPECT_EQ(simple_set.size(), 0);
		EXPECT_TRUE(simple_set.empty());
	}

	TEST_F(SparseSetTest, MoveConstructor) {
		simple_set.insert(index_0, TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_set.size(), 1);
		EXPECT_TRUE(simple_set.contains(index_0));

		SparseSet new_set(std::move(simple_set));
		EXPECT_EQ(new_set.size(), 1);
		EXPECT_TRUE(new_set.contains(index_0));
		EXPECT_EQ(new_set[index_0], TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_set.size(), 0);
		EXPECT_FALSE(simple_set.contains(index_0));
	}

	TEST_F(SparseSetTest, MoveOperator) {
		simple_set.insert(index_0, TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_set.size(), 1);
		EXPECT_TRUE(simple_set.contains(index_0));

		SparseSet<TestIndex, int> new_set;
		new_set = std::move(simple_set);
		EXPECT_EQ(new_set.size(), 1);
		EXPECT_TRUE(new_set.contains(index_0));
		EXPECT_EQ(new_set[index_0], TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_set.size(), 0);
		EXPECT_FALSE(simple_set.contains(index_0));
	}

	TEST_F(SparseSetTest, Contains) {
		simple_set.insert(index_0, TEST_EXPECTED_VALUE);
		simple_set.insert(index_1, TEST_EXPECTED_VALUE);

		EXPECT_TRUE(simple_set.contains(index_0));
		EXPECT_TRUE(simple_set.contains(index_1));

		simple_set.remove(index_0);
		EXPECT_FALSE(simple_set.contains(index_0));
		EXPECT_TRUE(simple_set.contains(index_1));

		simple_set.remove(index_1);
		EXPECT_FALSE(simple_set.contains(index_0));
		EXPECT_FALSE(simple_set.contains(index_1));
	}

	TEST_F(SparseSetTest, At) {
		simple_set.insert(index_0, TEST_EXPECTED_VALUE);
		simple_set.insert(index_1, TEST_EXPECTED_VALUE);

		EXPECT_EQ(simple_set[index_0], TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_set[index_1], TEST_EXPECTED_VALUE);

		const auto at_result = simple_set.at(index_0);
		ASSERT_TRUE(at_result.has_value());
		EXPECT_EQ(at_result->get(), TEST_EXPECTED_VALUE);

		const auto at_result_2 = simple_set.at(index_1);
		ASSERT_TRUE(at_result_2.has_value());
		EXPECT_EQ(at_result_2->get(), TEST_EXPECTED_VALUE);
	}

	TEST_F(SparseSetTest, AtConst) {
		simple_set.insert(index_0, TEST_EXPECTED_VALUE);
		simple_set.insert(index_1, TEST_EXPECTED_VALUE);

		const auto& const_simple_set = simple_set;

		EXPECT_EQ(const_simple_set[index_0], TEST_EXPECTED_VALUE);
		EXPECT_EQ(const_simple_set[index_1], TEST_EXPECTED_VALUE);

		const auto at_result = const_simple_set.at(index_0);
		ASSERT_TRUE(at_result.has_value());
		EXPECT_EQ(at_result->get(), TEST_EXPECTED_VALUE);

		const auto at_result_2 = const_simple_set.at(index_1);
		ASSERT_TRUE(at_result_2.has_value());
		EXPECT_EQ(at_result_2->get(), TEST_EXPECTED_VALUE);
	}

	TEST_F(SparseSetTest, Indices) {
		std::vector<TestIndex> indices(10);
		for (size_t i = 0; i < 10; ++i) {
			const TestIndex index{.value = i};
			indices[i] = index;
			simple_set.insert(index, i);
		}

		EXPECT_TRUE(std::ranges::equal(simple_set.indices(), indices));
	}

	TEST_F(SparseSetTest, Values) {
		std::vector<int> values(10);
		for (size_t i = 0; i < 10; ++i) {
			values[i] = i;
			simple_set.insert(TestIndex{.value = i}, i);
		}

		EXPECT_TRUE(std::ranges::equal(simple_set.values(), values));
	}

	TEST_F(SparseSetTest, Clear) {
		auto ptr = std::make_shared<int>();
		complex_set.insert(index_0, ptr);
		EXPECT_EQ(ptr.use_count(), 2);

		complex_set.clear();
		EXPECT_EQ(ptr.use_count(), 1);

		EXPECT_EQ(complex_set.size(), 0u);
		EXPECT_TRUE(complex_set.empty());
		EXPECT_FALSE(complex_set.contains(index_0));
		EXPECT_FALSE(complex_set.at(index_0).has_value());
	}

	TEST_F(SparseSetTest, Iter) {
		simple_set.insert(index_0, 10);
		simple_set.insert(index_1, 30);

		auto it = simple_set.iter();
		auto begin = it.begin();
		ASSERT_NE(begin, it.end());

		{
			auto [idx, val] = *begin;
			EXPECT_EQ(idx.to_index(), index_0.to_index());
			EXPECT_EQ(val, 10);
		}

		++begin;
		ASSERT_NE(begin, it.end());
		{
			auto [idx, val] = *begin;
			EXPECT_EQ(idx.to_index(), index_1.to_index());
			EXPECT_EQ(val, 30);
		}
	}
}