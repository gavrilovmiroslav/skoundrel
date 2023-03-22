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
		"create player-character1 with Position(x: 1, y: 2), Player;"
		"create player-character2 with Position(x: 12, y: 32), Player;"
		"foreach player with Position(x, y), Player { print(); }"
	);

	for (auto stat : statements)
	{
		stat->execute(ctx);
	}

	return 0;
}
