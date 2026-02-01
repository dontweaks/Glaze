#include "ECS/World.h"
#include <print>

#include "Utils/Layout.h"

struct Position {
	float x = 0.0f;
	float y = 0.0f;
};

struct Velocity {
	float x = 0.0f;
	float y = 0.0f;
};

int main() {
	using namespace glaze::ecs;

	World world;

	auto e1 = world.create_entity(
		Position{.x = 1.0f, .y = 10},
		Velocity{.x = 0.0f, .y = 0.0f}
	);

	constexpr glaze::utils::Layout l = glaze::utils::Layout::of<Position>();
	std::println("{}", l);

	//auto e2 = world.create_empty();
	//world.add_components(
	//	e2,
	//	Position{.x = 1.0f, .y = 10},
	//	Velocity{.x = 0.0f, .y = 0.0f}
	//);

	return 0;
}