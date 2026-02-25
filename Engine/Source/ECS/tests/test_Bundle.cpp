#include <gtest/gtest.h>

#include <print>
#include <utility>

#include "ECS/Bundle/BundleManager.h"

namespace glaze::ecs::tests {
	struct TestPosition {
		float x = 0.0f;
		float y = 0.0f;

		TestPosition(float x, float y) {
			this->x = x;
			this->y = y;
			std::println("Position(float x, float y)");
			std::flush(std::cout);
		}

		TestPosition() {
			std::println("Position()");
			std::flush(std::cout);
		}

		~TestPosition() {
			std::println("~Position()");
			std::flush(std::cout);
		}

		TestPosition(const TestPosition& v) {
			x = v.x;
			y = v.y;
			std::println("Position(const Position&)");
			std::flush(std::cout);
		}

		TestPosition(TestPosition&& v) {
			x = v.x;
			y = v.y;
			std::println("Position(Position&&)");
			std::flush(std::cout);
		}

		TestPosition& operator=(const TestPosition& v) {
			x = v.x;
			y = v.y;
			std::println("Position& operator=(const Position& v)");
			std::flush(std::cout);
		}

		TestPosition& operator=(TestPosition&& v) {
			x = v.x;
			y = v.y;
			std::println("Position& operator=(Position&& v)");
			std::flush(std::cout);
			return *this;
		}

		void print()
		{
			std::println("{}, {}", x, y);
		}
	};

	struct TestVelocity {
		static constexpr auto STORAGE_TYPE = StorageType::SparseSet;

		float x = 0.0f;
		float y = 0.0f;

		TestVelocity(float x, float y) {
			this->x = x;
			this->y = y;
			std::println("Velocity(float x, float y)");
			std::flush(std::cout);
		}

		TestVelocity() {
			std::println("Velocity()");
			std::flush(std::cout);
		}

		~TestVelocity() {
			std::println("~Velocity()");
			std::flush(std::cout);
		}

		TestVelocity(const TestVelocity& v) {
			x = v.x;
			y = v.y;
			std::println("Velocity(const Velocity&)");
			std::flush(std::cout);
		}

		TestVelocity(TestVelocity&& v) {
			x = v.x;
			y = v.y;
			std::println("Velocity(Velocity&&)");
			std::flush(std::cout);
		}

		TestVelocity& operator=(const TestVelocity& v) {
			x = v.x;
			y = v.y;
			std::println("Velocity& operator=(const Velocity& v)");
			std::flush(std::cout);
		}

		TestVelocity& operator=(TestVelocity&& v) {
			x = v.x;
			y = v.y;
			std::println("Velocity& operator=(Velocity&& v)");
			std::flush(std::cout);
		}

		void print()
		{
			std::println("{}, {}", x, y);
		}
	};

	struct TestBundle {
		TestPosition pos;
		TestVelocity velocity;

		auto components() && { return std::forward_as_tuple(std::move(pos), std::move(velocity)); }
		auto components() & = delete;
	};

	static_assert(Bundle<TestBundle>);
	static_assert(bundle_components_count<TestBundle>() == 2);
	static_assert(bundle_table_components_count<TestBundle>() == 1);
	static_assert(bundle_sparse_components_count<TestBundle>() == 1);

	struct InvalidBundle {
		TestPosition p;
		auto components() & { return p; }
	};
	static_assert(Bundle<InvalidBundle> == false);
}