#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600

#include <SDL2/SDL.h>
#include <entt/entt.hpp>
#include <libtcod.hpp>
#include <iostream>

#undef main 

#pragma once
#include <cstdio>
#include "ecs.h"
#include "parse.h"

using namespace std::chrono;

int main(int argc, char* argv[])
{
	ECS ecs;

	Context ctx;
	ctx.ecs = &ecs;

	auto statements = parse(
		"define Position(x: int, y: int);"
		"define Mass(kg: int);"
		"define Player;"
		"create player-character1 with Position(x: 1, y: 2), Mass(kg: 1), Player;"
		"create player-character2 with Position(x: 12, y: 32), Player;");

	for (auto stat : statements)
	{
		stat->execute(ctx);
	}

	for (auto e : ecs_query(ecs, { "Position", "Player" }, { "Mass" }))
	{
		auto& cp = ecs_get_component_by_instance(ecs, e, "Position");

		auto& x = ecs_get_member_in_component(ecs, cp, "x");
		auto& y = ecs_get_member_in_component(ecs, cp, "y");
		printf("%d %d\n", x.data.i.value, y.data.i.value);
	}

	return 0;
}
