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

	parse_file(ctx, "test.ska");

	ctx.update();

	return 0;
}
