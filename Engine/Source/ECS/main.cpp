#include "ECS/World.h"

#include <print>
#include <iostream>

#include "ECS/Storage/SparseSet.h"

struct Position {
	static constexpr auto STORAGE_TYPE = glaze::ecs::StorageType::SparseSet;

	float x = 0.0f;
	float y = 0.0f;

	Position(float x, float y) {
		this->x = x;
		this->y = y;
		std::println("Position(float x, float y)");
		std::flush(std::cout);
	}

	Position() {
		std::println("Position()");
		std::flush(std::cout);
	}

	~Position() {
		std::println("~Position()");
		std::flush(std::cout);
	}

	Position(const Position& v) {
		x = v.x;
		y = v.y;
		std::println("Position(const Position&)");
		std::flush(std::cout);
	}

	Position(Position&& v) {
		x = v.x;
		y = v.y;
		std::println("Position(Position&&)");
		std::flush(std::cout);
	}

	Position& operator=(const Position& v) {
		x = v.x;
		y = v.y;
		std::println("Position& operator=(const Position& v)");
		std::flush(std::cout);
	}

	Position& operator=(Position&& v) {
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

struct Velocity {
	float x = 0.0f;
	float y = 0.0f;

	Velocity(float x, float y) {
		this->x = x;
		this->y = y;
		std::println("Velocity(float x, float y)");
		std::flush(std::cout);
	}

	Velocity() {
		std::println("Velocity()");
		std::flush(std::cout);
	}

	~Velocity() {
		std::println("~Velocity()");
		std::flush(std::cout);
	}

	Velocity(const Velocity& v) {
		x = v.x;
		y = v.y;
		std::println("Velocity(const Velocity&)");
		std::flush(std::cout);
	}

	Velocity(Velocity&& v) {
		x = v.x;
		y = v.y;
		std::println("Velocity(Velocity&&)");
		std::flush(std::cout);
	}

	Velocity& operator=(const Velocity& v) {
		x = v.x;
		y = v.y;
		std::println("Velocity& operator=(const Velocity& v)");
		std::flush(std::cout);
	}

	Velocity& operator=(Velocity&& v) {
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

struct Render {
	int handle;

	void print() {
		std::println("{}", handle);
	}
};

using MyBundle = glaze::ecs::ComponentBundle<Position, Velocity>;

struct TestA {
	Position pos{1.0f, 2.0f};
	Velocity velocity{3.0f, 4.0f};

	auto components() && { return std::forward_as_tuple(std::move(pos), std::move(velocity)); }
	auto components() & = delete;
};

struct Index {
	[[nodiscard]] size_t to_index() const noexcept {
		return i;
	}

	uint32_t i;
};

int main() {
	using namespace glaze::ecs;

	World w;

	//Position p{1.0f, 1.0f};
	//Velocity v{1.0f, 1.0f};
	//w.create_entity(p, v);
	//w.create_entity(Position{}, Velocity{}, Render{});
	w.create_entity(TestA{});

	return 0;
}