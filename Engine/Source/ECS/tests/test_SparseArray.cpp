#include <gtest/gtest.h>
#include <string>

#include "ECS/Storage/SparseArray.h"

namespace glaze::ecs::tests {
	struct TestIndex {
		size_t value = 0;
		[[nodiscard]] constexpr size_t to_index() const noexcept { return value; }
		[[nodiscard]] constexpr auto operator<=>(const TestIndex& index) const noexcept = default;
	};

	struct SparseArrayTest : testing::Test {
	protected:
		static constexpr size_t PAGE_SIZE = 4;
		SparseArray<TestIndex, int, PAGE_SIZE> simple_array;
		SparseArray<TestIndex, std::shared_ptr<int>, PAGE_SIZE> complex_array;
		TestIndex page_one_index_0{0};
		TestIndex page_one_index_1{1};
		TestIndex page_two_index_0{4};
		TestIndex page_two_index_1{5};
		static inline int TEST_EXPECTED_VALUE{10};
		static inline int TEST_EXPECTED_VALUE_OVERRIDE{20};
	};

	TEST_F(SparseArrayTest, Empty) {
		EXPECT_EQ(simple_array.size(), 0);
		EXPECT_EQ(simple_array.page_count(), 0);
		EXPECT_TRUE(simple_array.empty());
	}

	TEST_F(SparseArrayTest, EmplacePageOne) {
		{
			const auto& value = simple_array.emplace(page_one_index_0, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
			EXPECT_EQ(simple_array.size(), 1);
			EXPECT_EQ(simple_array.page_count(), 1);
			EXPECT_FALSE(simple_array.empty());
		}

		{
			const auto& value = simple_array.emplace(page_one_index_1, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
			EXPECT_EQ(simple_array.size(), 2);
			EXPECT_EQ(simple_array.page_count(), 1);
			EXPECT_FALSE(simple_array.empty());
		}
	}

	TEST_F(SparseArrayTest, EmplacePageTwo) {
		{
			const auto& value = simple_array.emplace(page_two_index_0, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
			EXPECT_EQ(simple_array.size(), 1);
			EXPECT_EQ(simple_array.page_count(), 2);
			EXPECT_FALSE(simple_array.empty());
		}
		{
			const auto& value = simple_array.emplace(page_one_index_1, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
			EXPECT_EQ(simple_array.size(), 2);
			EXPECT_EQ(simple_array.page_count(), 2);
			EXPECT_FALSE(simple_array.empty());
		}
	}

	TEST_F(SparseArrayTest, EmplaceOverride) {
		{
			const auto& value = simple_array.emplace(page_one_index_0, TEST_EXPECTED_VALUE);
			EXPECT_EQ(value, TEST_EXPECTED_VALUE);
		}
		{
			const auto& value_overriden = simple_array.emplace(page_one_index_0, TEST_EXPECTED_VALUE_OVERRIDE);
			EXPECT_EQ(value_overriden, TEST_EXPECTED_VALUE_OVERRIDE);
		}
		EXPECT_EQ(simple_array.size(), 1);
		EXPECT_EQ(simple_array.page_count(), 1);
		EXPECT_FALSE(simple_array.empty());
	}

	TEST_F(SparseArrayTest, Insert) {
		auto ptr = std::make_shared<int>();
		complex_array.insert(page_one_index_0, ptr);
		EXPECT_EQ(ptr.use_count(), 2);

		complex_array.insert(page_one_index_1, std::move(ptr));
		EXPECT_EQ(ptr.use_count(), 0);
		EXPECT_FALSE(ptr);
	}

	TEST_F(SparseArrayTest, Remove) {
		simple_array.insert(page_one_index_0, TEST_EXPECTED_VALUE);
		simple_array.insert(page_one_index_1, TEST_EXPECTED_VALUE);
		const auto rem_result = simple_array.remove(page_one_index_0);
		EXPECT_EQ(simple_array.size(), 1);
		EXPECT_EQ(simple_array.page_count(), 1);

		EXPECT_TRUE(rem_result.has_value());
		EXPECT_EQ(rem_result.value(), TEST_EXPECTED_VALUE);

		const auto rem_result_2 = simple_array.remove(page_one_index_1);
		EXPECT_EQ(simple_array.size(), 0);
		EXPECT_EQ(simple_array.page_count(), 0);
		EXPECT_TRUE(simple_array.empty());

		EXPECT_TRUE(rem_result_2.has_value());
		EXPECT_EQ(rem_result_2.value(), TEST_EXPECTED_VALUE);
	}

	TEST_F(SparseArrayTest, MoveConstructor) {
		simple_array.insert(page_one_index_0, TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_array.size(), 1);
		EXPECT_EQ(simple_array.page_count(), 1);
		EXPECT_TRUE(simple_array.contains(page_one_index_0));

		SparseArray new_array(std::move(simple_array));
		EXPECT_EQ(simple_array.page_count(), 0);
		EXPECT_FALSE(simple_array.contains(page_one_index_0));

		EXPECT_EQ(new_array.size(), 1);
		EXPECT_EQ(new_array.page_count(), 1);
		EXPECT_TRUE(new_array.contains(page_one_index_0));
	}

	TEST_F(SparseArrayTest, MoveOperator) {
		simple_array.insert(page_one_index_0, TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_array.size(), 1);
		EXPECT_EQ(simple_array.page_count(), 1);
		EXPECT_TRUE(simple_array.contains(page_one_index_0));

		SparseArray<TestIndex, int, PAGE_SIZE> new_array;
		new_array = std::move(simple_array);
		EXPECT_EQ(simple_array.page_count(), 0);
		EXPECT_FALSE(simple_array.contains(page_one_index_0));

		EXPECT_EQ(new_array.size(), 1);
		EXPECT_EQ(new_array.page_count(), 1);
		EXPECT_TRUE(new_array.contains(page_one_index_0));
	}

	TEST_F(SparseArrayTest, Contains) {
		simple_array.insert(page_one_index_0, TEST_EXPECTED_VALUE);
		simple_array.insert(page_one_index_1, TEST_EXPECTED_VALUE);

		EXPECT_TRUE(simple_array.contains(page_one_index_0));
		EXPECT_TRUE(simple_array.contains(page_one_index_1));

		simple_array.remove(page_one_index_0);
		EXPECT_FALSE(simple_array.contains(page_one_index_0));
		EXPECT_TRUE(simple_array.contains(page_one_index_1));

		simple_array.remove(page_one_index_1);
		EXPECT_FALSE(simple_array.contains(page_one_index_0));
		EXPECT_FALSE(simple_array.contains(page_one_index_1));
	}

	TEST_F(SparseArrayTest, At) {
		simple_array.insert(page_one_index_0, TEST_EXPECTED_VALUE);
		simple_array.insert(page_one_index_1, TEST_EXPECTED_VALUE);

		EXPECT_EQ(simple_array[page_one_index_0], TEST_EXPECTED_VALUE);
		EXPECT_EQ(simple_array[page_one_index_1], TEST_EXPECTED_VALUE);

		const auto at_result = simple_array.at(page_one_index_0);
		EXPECT_TRUE(at_result.has_value());
		EXPECT_EQ(at_result.value(), TEST_EXPECTED_VALUE);

		const auto at_result_2 = simple_array.at(page_one_index_1);
		EXPECT_TRUE(at_result_2.has_value());
		EXPECT_EQ(at_result_2.value(), TEST_EXPECTED_VALUE);
	}

	TEST_F(SparseArrayTest, AtConst) {
		simple_array.insert(page_one_index_0, TEST_EXPECTED_VALUE);
		simple_array.insert(page_one_index_1, TEST_EXPECTED_VALUE);

		const auto& const_simple_array = simple_array;

		EXPECT_EQ(const_simple_array[page_one_index_0], TEST_EXPECTED_VALUE);
		EXPECT_EQ(const_simple_array[page_one_index_1], TEST_EXPECTED_VALUE);

		const auto at_result = const_simple_array.at(page_one_index_0);
		EXPECT_TRUE(at_result.has_value());
		EXPECT_EQ(at_result.value(), TEST_EXPECTED_VALUE);

		const auto at_result_2 = const_simple_array.at(page_one_index_1);
		EXPECT_TRUE(at_result_2.has_value());
		EXPECT_EQ(at_result_2.value(), TEST_EXPECTED_VALUE);
	}

	TEST_F(SparseArrayTest, Clear) {
		auto ptr = std::make_shared<int>();
		complex_array.insert(page_one_index_0, ptr);
		EXPECT_EQ(ptr.use_count(), 2);

		complex_array.clear();
		EXPECT_EQ(ptr.use_count(), 1);

		EXPECT_EQ(complex_array.size(), 0);
		EXPECT_EQ(complex_array.page_count(), 0);
		EXPECT_TRUE(complex_array.empty());
		EXPECT_FALSE(complex_array.contains(page_one_index_0));
	}
}