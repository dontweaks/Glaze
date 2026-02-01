#include "ECS/World.h"

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

	return 0;
}